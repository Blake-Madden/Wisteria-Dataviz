/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef PDF_EXTRACT_TEXT_H
#define PDF_EXTRACT_TEXT_H

#include "extract_text.h"
#include "pdf_decrypt.h"
#include "pdf_document.h"
#include "pdf_lexer.h"
#include <cstdint>
#include <exception>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace lily_of_the_valley
    {
    /// @brief Text encoding and CMap decoding utilities.
    class pdf_text_decoder
        {
      public:
        /// @returns The Unicode equivalent of a WinAnsi (CP1252) byte.
        [[nodiscard]]
        static wchar_t cp1252_to_unicode(unsigned char byteValue);

        /// @returns The Unicode equivalent of a MacRomanEncoding byte.
        [[nodiscard]]
        static wchar_t mac_roman_to_unicode(unsigned char byteValue);

        /// @returns The Unicode equivalent of an (Adobe) StandardEncoding byte.
        [[nodiscard]]
        static wchar_t standard_to_unicode(unsigned char byteValue);

        /// @returns The Unicode equivalent of an Adobe SymbolEncoding byte
        ///     (the built-in encoding of the standard Symbol font).
        [[nodiscard]]
        static wchar_t symbol_to_unicode(unsigned char byteValue);

        /// @returns The Unicode equivalent of an Adobe ZapfDingbatsEncoding byte
        ///     (the built-in encoding of the standard ZapfDingbats font).
        [[nodiscard]]
        static wchar_t zapf_dingbats_to_unicode(unsigned char byteValue);

        /// @brief Converts UTF-16BE code units to a wide string.
        /// @details On 32-bit @c wchar_t platforms, surrogate pairs are combined into
        ///     single code points. On 16-bit platforms (Windows), valid pairs are emitted
        ///     as two units. Lone surrogates are dropped on both platforms.
        [[nodiscard]]
        static std::wstring utf16_units_to_wstring(const std::vector<char16_t>& units);

        /// @brief Converts hex digits to UTF-16BE code units (as used in ToUnicode CMaps).
        [[nodiscard]]
        static std::vector<char16_t> hex_to_utf16_units(std::string_view hexDigits);

        /// @brief Converts hex digits to an unsigned integer (capped at 8 digits).
        [[nodiscard]]
        static uint32_t hex_to_uint(std::string_view hexDigits);

        /// @brief Decodes a metadata string's bytes (UTF-16BE if it has a BOM,
        ///     WinAnsi/Latin-1 otherwise).
        [[nodiscard]]
        static std::wstring decode_metadata_string(std::string_view bytes);

        /// @brief Parses a font's ToUnicode CMap (bfchar/bfrange sections) into a decoder.
        static void parse_unicode_cmap(std::string_view cmap, pdf_font_decoder& decoder);

        /// @brief Parses one of Adobe's CID CMap resource files (cidchar/cidrange
        ///     sections) into a CID-to-Unicode table.
        /// @details These files map Unicode code points to CIDs, the direction needed
        ///     to encode text with such a font. The goal here is the opposite: recovering
        ///     Unicode text from a font's CIDs, for fonts with no ToUnicode CMap of their
        ///     own. Each entry is inverted as it's parsed to support that.
        /// @param cmap The CMap resource file's raw text content.
        /// @param table The table to populate.
        /// @sa https://github.com/adobe-type-tools/cmap-resources
        static void parse_cid_to_unicode_cmap(std::string_view cmap, cid_to_unicode_table& table);

        /// @brief Extracts the CMap name referenced by an embedded CMap stream's
        ///     `/<Name> usecmap` directive, if any.
        /// @param cmap The raw (decoded) content of an embedded CMap stream.
        /// @returns The referenced CMap's name, without the leading slash, or empty
        ///     if the stream has no `usecmap` directive.
        [[nodiscard]]
        static std::string_view parse_usecmap_name(std::string_view cmap);

        /// @brief Reads an embedded CMap stream's own `/WMode` directive, if any.
        /// @param cmap The raw (decoded) content of an embedded CMap stream.
        /// @returns @c true if the stream declares `/WMode 1` (vertical writing
        ///     mode). A subsetted CID font's own CMap may declare this directly,
        ///     independently of (or in addition to) chaining to a predefined CMap
        ///     via usecmap.
        [[nodiscard]]
        static bool parse_wmode_from_cmap_stream(std::string_view cmap);

        /// @brief Maps the name of one of Adobe's predefined CJK CMaps (a Type0
        ///     font's /Encoding value, without the leading slash) to the legacy
        ///     multibyte charset that strings shown with such a font are encoded in.
        /// @param cmapName The CMap name (e.g., "ETenms-B5-H" or "90ms-RKSJ-V").
        /// @returns The charset's name (e.g., "CP950"), as understood by a
        ///     charset_convert_functor, or empty if @c cmapName is not a
        ///     recognized predefined CJK CMap.
        [[nodiscard]]
        static std::string_view predefined_cmap_charset(std::string_view cmapName);

        /// @returns @c true if @c cmapName (a Type0 font's /Encoding value, without
        ///     the leading slash) is one of Adobe's predefined Unicode CMaps (e.g.,
        ///     "UniJIS-UCS2-H"), meaning the font's character codes are UTF-16BE
        ///     code units.
        [[nodiscard]]
        static bool is_unicode_cmap_name(std::string_view cmapName);

        /// @returns @c true if @c cmapName (a Type0 font's /Encoding value, without
        ///     the leading slash) is one of Adobe's predefined vertical-writing-mode
        ///     CMaps (e.g., "Identity-V" or "UniJIS-UCS2-V").
        [[nodiscard]]
        static bool is_vertical_cmap_name(std::string_view cmapName);

        /// @brief Resolves a PDF glyph name (from a `/Differences` array) to Unicode.
        /// @param glyphName The (ASCII) glyph name, without the leading slash.
        /// @param glyphTable A glyph name table (e.g., the Adobe Glyph List) to consult first.
        /// @returns The Unicode text for the glyph, falling back to the `uniXXXX`/`uXXXXXX`
        ///     naming convention if not found in @c glyphTable. Empty if unresolvable.
        [[nodiscard]]
        static std::wstring glyph_name_to_unicode(std::string_view glyphName,
                                                  const glyph_name_table& glyphTable);

        /// @brief Parses a simple font's `/Differences` array (e.g.,
        ///     `[ 39 /quotesingle 96 /grave ]`) into a decoder.
        /// @param differencesArray The raw `/Differences` array value (including brackets).
        /// @param glyphTable A glyph name table (e.g., the Adobe Glyph List) to resolve
        ///     each glyph name with.
        static void parse_differences_array(std::string_view differencesArray,
                                            const glyph_name_table& glyphTable,
                                            pdf_font_decoder& decoder);

        /// @brief Parses a simple TrueType font's embedded `cmap` table (its
        ///     `/FontFile2` program) to recover a code -> Unicode mapping, used as a
        ///     fallback when the font has no `/ToUnicode` CMap and no (resolvable)
        ///     `/Differences` array.
        /// @details Chains the cmap's (1, 0) [Mac Roman] format 0 subtable (code ->
        ///     glyph index) with its (3, 1) [Windows Unicode BMP] format 4 subtable,
        ///     inverted (glyph index -> Unicode), to recover each code's Unicode value.
        ///     Other subtable formats and platform/encoding combinations are ignored.
        /// @param fontProgram The raw, decoded bytes of the font's `/FontFile2` stream.
        /// @param decoder The decoder to populate m_code_map for.
        static void parse_embedded_truetype_cmap(std::string_view fontProgram,
                                                 pdf_font_decoder& decoder);

        /// @brief Parses a CID-keyed TrueType font's embedded `cmap` table to
        ///     recover a CID -> Unicode mapping, used as a fallback when a
        ///     `/Type0` font's descendant `/CIDFontType2` has no `/ToUnicode` CMap.
        /// @param fontProgram The raw, decoded bytes of the font's `/FontFile2` stream.
        /// @param cidToGidMapData The descendant font's decoded `/CIDToGIDMap`
        ///     stream bytes, or empty for `/Identity` (the default).
        /// @param decoder The decoder to populate m_code_map for.
        static void parse_embedded_cid_truetype_cmap(std::string_view fontProgram,
                                                     std::string_view cidToGidMapData,
                                                     pdf_font_decoder& decoder);

        /// @brief Determines how many bytes (starting at @c pos) make up the next
        ///     character code, per the font's declared codespace ranges.
        /// @details If @c fontDecoder has one or more codespace ranges, the leading
        ///     bytes at @c pos are compared against each range's bounds (in declared
        ///     order); the first range whose bounds contain those bytes determines
        ///     the length. If none match (or no ranges were declared), this falls
        ///     back to @c fontDecoder->m_bytes_per_code.
        /// @returns The code length, in bytes (at least 1, and clipped to the
        ///     remaining bytes in @c bytes).
        [[nodiscard]]
        static size_t determine_code_length(const std::string& bytes, size_t pos,
                                            const pdf_font_decoder* fontDecoder);

        /// @brief Decodes a content-stream string's bytes to Unicode using a font decoder.
        [[nodiscard]]
        static std::wstring decode_string_bytes(const std::string& bytes,
                                                const pdf_font_decoder* fontDecoder);

      private:
        /// @brief Reads a big-endian @c uint16_t from TrueType binary data.
        /// @returns The value, or 0 if @c pos is out of range for @c data.
        [[nodiscard]]
        static uint16_t truetype_read_uint16(std::string_view data, size_t pos);

        /// @brief Reads a big-endian @c uint32_t from TrueType binary data.
        /// @returns The value, or 0 if @c pos is out of range for @c data.
        [[nodiscard]]
        static uint32_t truetype_read_uint32(std::string_view data, size_t pos);

        /// @brief Locates the "cmap" table in an sfnt font program.
        /// @returns @c true (filling in @c cmapTableOut) if found.
        [[nodiscard]]
        static bool find_cmap_table(std::string_view fontProgram, std::string_view& cmapTableOut);

        /// @returns @c true (filling in @c offsetOut) if @c cmapTable has a
        ///     subtable for the given platform/encoding ID pair.
        [[nodiscard]]
        static bool find_cmap_subtable_offset(std::string_view cmapTable, uint16_t platformID,
                                              uint16_t encodingID, size_t& offsetOut);

        /// @brief Inverts a cmap format 4 (segmented Unicode BMP) subtable into
        ///     glyph index -> Unicode.
        /// @returns The inverted map, empty if not a valid format 4 subtable.
        [[nodiscard]]
        static std::unordered_map<uint32_t, wchar_t>
        invert_format4_unicode_subtable(std::string_view cmapTable, size_t subtableOffset);
        };

    /** @brief Functor for applying Unicode normalization to the final extracted text.
        @details Compatibility composition (NFKC) is the intended use: it folds
            ligatures, full-width/half-width forms, and other formatting-only
            distinctions (which a font's ToUnicode CMap or glyph table may
            introduce) into their canonical plain-text equivalents, so that
            visually-equivalent text compares and searches consistently.
        @param text The extracted text to normalize.
        @returns The normalized text.*/
    using unicode_normalize_functor = std::function<std::wstring(std::wstring_view text)>;

    /** @brief Class to extract text from a <b>PDF</b> stream.
        @details Raw text is extracted from the document's page content streams,
            preserving basic structural formatting. This will format line breaks,
            paragraph breaks, and bulleted list items as a newline, followed by
            a tab and the bullet character.

            Text encoding is resolved through each font's embedded @c ToUnicode CMap
            (when available); single-byte codes, otherwise, are mapped through
            CP1252/Latin-1. CJK text shown with one of Adobe's predefined Unicode
            CMap encodings (e.g., @c UniJIS-UCS2-H) is decoded directly; the
            predefined legacy CJK CMap encodings (e.g., @c ETenms-B5-H) additionally
            require connecting a charset converter via set_charset_converter().

            Most PDF files compress their content streams (usually zlib/DEFLATE
            compression [@c FlateDecode]). This class is decompression-library
            agnostic; call set_stream_decompressor() to connect a function object that
            performs the decompression. (Without a decompressor, only uncompressed
            content can be extracted.)

            Extracted text is left as-is unless a normalizer is connected via
            set_normalizer(). Connecting one is recommended, to fold compatibility
            variants (ligatures, full-width forms, etc.) into canonical plain text.
        @par Example:
        @code
        std::ifstream fs("C:\\users\\daphne\\physical-therapy-instructions.pdf",
                         std::ios::in|std::ios::binary|std::ios::ate);
        if (fs.is_open())
            {
            // read a PDF file into a char* buffer
            size_t fileSize = fs.tellg();
            std::vector<char> fileContents(fileSize);
            fs.seek(0, std::ios::beg);
            fs.read(fileContents.data(), fileSize);
            // convert the PDF data into raw text
            lily_of_the_valley::pdf_extract_text pdfExtract;
            // optionally, connect a zlib decompressor so that
            // compressed content streams can be read:
            // pdfExtract.set_stream_decompressor(...);
            pdfExtract(fileContents.data(), fileSize);
            // the raw text from the file is now in a Unicode buffer
            std::wstring fileText(pdfExtract.get_filtered_text(),
                                  pdfExtract.get_filtered_text_length());
            }
        @endcode*/
    class pdf_extract_text final : public extract_text
        {
      public:
        /** @brief Connects a function object used to decompress @c FlateDecode
                (i.e., zlib) stream sections.
            @param decompressFunction The decompression functor to use.
            @sa stream_decompress_functor.*/
        void set_stream_decompressor(stream_decompress_functor decompressFunction)
            {
            m_decompress = std::move(decompressFunction);
            }

        /** @brief Connects a function object used to convert legacy multibyte
                charset bytes (e.g., Big5, Shift-JIS) to Unicode.
            @details This is used to decode Type0 fonts whose /Encoding is one of
                Adobe's predefined CJK CMaps (e.g., /ETenms-B5-H); such fonts'
                string bytes are text in the CMap's underlying charset, and they
                typically have no ToUnicode CMap to decode them with instead.
                (Without a converter, text shown with such fonts is dropped.)
            @param convertFunction The charset conversion functor to use.
            @sa charset_convert_functor.*/
        void set_charset_converter(charset_convert_functor convertFunction)
            {
            m_charset_convert = std::move(convertFunction);
            }

        /** @brief Connects a function object used to perform AES-CBC
                encryption/decryption.
            @details Required to read AES-encrypted documents: `/CFM` `/AESV2`
                (`/V` 4) or `/AESV3` (`/V` 5, revisions 5/6).
            @param aesFunction The AES-CBC functor to use.
            @sa aes_cbc_functor.*/
        void set_aes_decryptor(aes_cbc_functor aesFunction)
            {
            m_aes_decrypt = std::move(aesFunction);
            }

        /** @brief Connects a function object used to compute SHA-2 (256/384/512)
                message digests.
            @details Required to read documents encrypted with revision 5 or 6
                (`/V` 5, AES-256) security handlers, whose key derivation is
                SHA-256-based (revision 6 additionally hardens this with a
                SHA-256/384/512 + AES-128 loop).
            @param hashFunction The SHA-2 functor to use.
            @sa sha2_functor.*/
        void set_hash_functor(sha2_functor hashFunction) { m_hash = std::move(hashFunction); }

        /** @brief Connects a function object used to apply Unicode normalization
                (e.g., NFKC) to the final extracted text.
            @details Optional; without a normalizer, extracted text keeps whatever
                compatibility variants a font's encoding introduced (ligatures,
                full-width forms, etc.) instead of folding them to canonical text.
            @param normalizeFunction The normalization functor to use.
            @sa unicode_normalize_functor.*/
        void set_normalizer(unicode_normalize_functor normalizeFunction)
            {
            m_normalize = std::move(normalizeFunction);
            }

        /** @brief Loads a glyph name table (e.g., the Adobe Glyph List) used to resolve
                simple fonts' `/Differences` custom encodings.
            @details The table is expected in the same format as Adobe's AGL data files:
                semicolon-delimited lines of `glyphname;XXXX[ XXXX...]`, where each `XXXX`
                is a four-digit hexadecimal Unicode value; lines starting with `#`
                (and blank lines) are ignored.\n
                The table is shared (process-wide) across every instance of this class,
                so this only needs to be called once; every instance will see it, including
                ones already constructed.
            @param text The glyph table's text content.
            @sa https://github.com/adobe-type-tools/agl-aglfn */
        static void load_glyph_name_table(std::wstring_view text);

        /** @brief Loads a CID-to-Unicode table for a specific Adobe CID-keyed
                character collection (e.g., "Adobe-Japan1"), used to resolve Type0/CID
                fonts (common for CJK text) that have no embedded ToUnicode CMap of
                their own. (This is optional.) Call this once for each character
                collection you need to support.
            @details The table is expected in the same format as one of Adobe's CMap
                resource files (e.g., @c UniJIS-UTF16-H, for "Adobe-Japan1").\n
                Tables are shared (process-wide) across every instance of this class,
                so each collection only needs to be loaded once; every instance will
                see it, including ones already constructed.
            @param registryOrdering The character collection this table applies to, as
                `Registry-Ordering` (e.g., "Adobe-Japan1"), matching a font's
                `/CIDSystemInfo` dictionary.
            @param text The CMap resource file's text content.
            @sa https://github.com/adobe-type-tools/cmap-resources */
        static void load_cid_to_unicode_table(std::string_view registryOrdering,
                                              std::wstring_view text);

        /** @brief Main interface for extracting plain text from a PDF buffer.
            @param pdf_buffer The PDF stream to convert to plain text.
            @param text_length The length of the PDF buffer.
            @returns A pointer to the parsed text, or @c nullptr upon failure.\n
                Call get_filtered_text_length() to get the length of the parsed text.
            @throws pdf_header_not_found If an invalid document.
            @throws pdf_encrypted If the document is encrypted
                (and hence cannot be parsed).*/
        const wchar_t* operator()(const char* pdf_buffer, size_t text_length);

        /** @returns The title from the document's metadata.
            @note Must be called after calling @c operator().*/
        [[nodiscard]]
        const std::wstring& get_title() const noexcept
            {
            return m_title;
            }

        /** @returns The author from the document's metadata.
            @note Must be called after calling @c operator().*/
        [[nodiscard]]
        const std::wstring& get_author() const noexcept
            {
            return m_author;
            }

        /** @returns The subject from the document's metadata.
            @note Must be called after calling @c operator().*/
        [[nodiscard]]
        const std::wstring& get_subject() const noexcept
            {
            return m_subject;
            }

        /** @returns The keywords from the document's metadata.
            @note Must be called after calling @c operator().*/
        [[nodiscard]]
        const std::wstring& get_keywords() const noexcept
            {
            return m_keywords;
            }

        /// @brief Exception thrown when a PDF file is missing its header
        ///     (more than likely an invalid PDF file).
        class pdf_header_not_found : public std::exception
            {
            };

        /// @brief Exception thrown when a PDF file is encrypted
        ///     (encrypted documents are not supported).
        class pdf_encrypted : public std::exception
            {
            };

      private:
        /// @brief Reads a string value (which may be an indirect reference to a
        ///     string object) into Unicode text.
        /// @param document The document (used to resolve indirect references and
        ///     decrypt, if applicable).
        /// @param owningObject The indirect object that directly contains @c value
        ///     (used to derive its decryption key, if the document is encrypted).
        /// @param value The (possibly indirect) string value to read.
        [[nodiscard]]
        static std::wstring read_string_value(const pdf_document& document,
                                              const pdf_object& owningObject,
                                              std::string_view value);

        /// @brief Gathers the trailer dictionaries (both classic `trailer` sections and
        ///     cross-reference stream dictionaries), which hold the /Root, /Info,
        ///     and /Encrypt entries.
        [[nodiscard]]
        static std::vector<std::string_view> collect_trailers(std::string_view fileContent,
                                                              const pdf_document& document);

        /// @brief Recursively walks the page tree, collecting page objects in
        ///     document order.
        static void walk_page_tree(const pdf_document& document, long nodeNumber,
                                   std::set<long>& visitedNodes, std::vector<long>& pageOrder,
                                   int depth);

        /// @brief Reads a (possibly `(literal)` or `<hex>`) string's raw bytes.
        /// @returns The decoded raw bytes, or an empty string if @c value isn't a string.
        [[nodiscard]]
        static std::string read_raw_string(std::string_view value);

        /// @brief Attempts to build a decryptor from the trailer's `/Encrypt` and `/ID`
        ///     entries, assuming an empty user password.
        /// @param document The document (used to resolve `/Encrypt`, if it's an
        ///     indirect reference).
        /// @param encryptValue The trailer's raw `/Encrypt` value.
        /// @param idValue The trailer's raw `/ID` value (its array, not just one string).
        /// @param aesFunction The AES-CBC functor connected via set_aes_decryptor()
        ///     (may be empty).
        /// @param hashFunction The SHA-2 functor connected via set_hash_functor()
        ///     (may be empty).
        /// @returns The decryptor, or @c nullptr if the document doesn't use a
        ///     supported encryption scheme, or authentication failed.
        [[nodiscard]]
        static std::unique_ptr<pdf_decryptor>
        setup_decryption(const pdf_document& document, std::string_view encryptValue,
                         std::string_view idValue, const aes_cbc_functor& aesFunction,
                         const sha2_functor& hashFunction);

        stream_decompress_functor m_decompress;    ///< FlateDecode decompression functor.
        charset_convert_functor m_charset_convert; ///< Legacy CJK charset conversion functor.
        aes_cbc_functor m_aes_decrypt;             ///< AES-CBC encryption/decryption functor.
        sha2_functor m_hash;                       ///< SHA-2 hashing functor.
        unicode_normalize_functor m_normalize;     ///< Unicode normalization (e.g., NFKC) functor.
        /// @brief Glyph names for `/Differences` resolution.
        /// @details Shared (process-wide) across every instance; loaded once via
        ///     load_glyph_name_table() and only read from (never mutated) while
        ///     parsing a document.
        inline static glyph_name_table m_glyph_name_table;
        /// @brief CID-to-Unicode tables, by ordering.
        /// @details Shared (process-wide) across every instance; loaded once via
        ///     load_cid_to_unicode_table() and only read from (never mutated) while
        ///     parsing a document.
        inline static cid_to_unicode_registry m_cid_to_unicode_tables;
        std::wstring m_title;    ///< Document title from /Info metadata.
        std::wstring m_author;   ///< Document author from /Info metadata.
        std::wstring m_subject;  ///< Document subject from /Info metadata.
        std::wstring m_keywords; ///< Document keywords from /Info metadata.
        };

    } // namespace lily_of_the_valley

#endif // PDF_EXTRACT_TEXT_H
