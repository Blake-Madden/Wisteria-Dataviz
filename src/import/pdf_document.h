/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef PDF_DOCUMENT_H
#define PDF_DOCUMENT_H

#include "pdf_decrypt.h"
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace lily_of_the_valley
    {
    /// @brief An indirect object from a PDF file.
    struct pdf_object
        {
        /// The object's dictionary (empty if it doesn't have one).
        std::string_view m_dictionary;
        /// The raw (possibly compressed) stream payload (if a stream object).
        std::string_view m_stream_data;
        /// The object's value, if not a dictionary.
        std::string_view m_value;
        /// The object's number, from its "N G obj" header.
        long m_object_number{ 0 };
        /// The object's generation number, from its "N G obj" header.
        /// @details Used for the standard PDF security handler's per-object key
        ///     derivation. Always 0 for objects packed inside an object stream
        ///     (`/ObjStm`), since the PDF spec requires those to be generation 0.
        long m_generation{ 0 };
        /// @brief Whether this object was unpacked from a compressed object
        ///     stream (`/ObjStm`).
        /// @details Such objects are never individually encrypted (the containing
        ///     stream was decrypted as a whole), so their strings must not be run
        ///     through decryption a second time.
        bool m_from_object_stream{ false };
        };

    /// @brief A glyph name -> Unicode mapping, such as the Adobe Glyph List, used to
    ///     resolve a simple font's `/Differences` custom encoding.
    /// @sa pdf_extract_text::load_glyph_name_table().
    using glyph_name_table = std::unordered_map<std::wstring, std::wstring>;

    /// @brief A CID -> Unicode mapping for one Adobe CID-keyed character collection
    ///     (e.g., "Adobe-Japan1"), used to resolve a Type0/CID font that has no
    ///     embedded ToUnicode CMap of its own.
    /// @sa pdf_extract_text::load_cid_to_unicode_table().
    using cid_to_unicode_table = std::unordered_map<uint32_t, std::wstring>;

    /// @brief A set of cid_to_unicode_table objects, keyed by the character
    ///     collection (`Registry-Ordering`, e.g. "Adobe-Japan1") each applies to.
    using cid_to_unicode_registry = std::map<std::string, cid_to_unicode_table, std::less<>>;

    /** @brief Function object interface used to convert legacy multibyte charset
            bytes (e.g., Big5, Shift-JIS) to Unicode.
        @details The functor is passed a string's raw bytes and the name of the
            charset they are encoded in ("CP932", "CP936", "CP949", "CP950",
            "GB2312", "GB18030", "EUC-JP", or "EUC-KR") and should return the
            equivalent Unicode text. Used to decode Type0 fonts that use one of
            Adobe's predefined CJK CMaps (e.g., /ETenms-B5-H) as their encoding,
            since those fonts' string bytes are text in the CMap's underlying
            charset.
        @returns An empty string upon failure.*/
    using charset_convert_functor = std::function<std::wstring(std::string_view, std::string_view)>;

    /// @brief A per-font decoder that maps character codes from a
    ///     content stream to Unicode.
    struct pdf_font_decoder
        {
        /// @brief The base (single-byte) encoding to fall back to when a code has no
        ///     entry in @c m_code_map.
        enum class base_encoding_type
            {
            win_ansi,  ///< WinAnsiEncoding (CP1252); the default when no /Encoding is given.
            mac_roman, ///< MacRomanEncoding.
            standard   ///< (Adobe) StandardEncoding.
            };

        /// @brief A single entry from a ToUnicode CMap's `/codespacerange`, defining the
        ///     byte length and valid byte bounds for codes of that length.
        /// @details A CMap may declare more than one of these (e.g., 1-byte codes for
        ///     ASCII mixed with 2-byte codes for CJK glyphs); the low/high bounds and
        ///     byte length are compared against the leading bytes of a content-stream
        ///     string to determine how many bytes make up the next code.
        struct codespace_range
            {
            uint32_t m_low{ 0 };
            uint32_t m_high{ 0 };
            size_t m_byte_length{ 1 };
            };

        /// Character code -> Unicode mapping (from the font's ToUnicode CMap and/or
        /// its /Differences array).
        std::map<uint32_t, std::wstring> m_code_map;
        /// The codespace ranges declared by the font's ToUnicode CMap (if any); used to
        /// determine the byte length of each code when it varies across the string
        /// (e.g., a mix of 1-byte and 2-byte codes). Empty if the CMap declared none
        /// (or there is no ToUnicode CMap), in which case @c m_bytes_per_code is used
        /// as a fixed width for the whole string.
        std::vector<codespace_range> m_codespace_ranges;
        /// The width (in bytes) of the font's character codes, used as a fallback
        /// when @c m_codespace_ranges is empty or a code doesn't match any range in it.
        size_t m_bytes_per_code{ 1 };
        /// The base encoding for single-byte codes with no @c m_code_map entry.
        base_encoding_type m_base_encoding{ base_encoding_type::win_ansi };
        /// The legacy multibyte charset (e.g., "CP950" for Big5) implied by the font
        /// using one of Adobe's predefined CJK CMaps as its /Encoding (e.g.,
        /// /ETenms-B5-H); such a font's string bytes are text in that charset.
        /// Empty for all other fonts.
        std::string m_charset;
        /// The charset converter to decode @c m_charset bytes with
        /// (a copy of the one connected via
        /// pdf_extract_text::set_charset_converter(), if any).
        charset_convert_functor m_charset_converter;
        /// Whether @c m_code_map has any entries (from a ToUnicode CMap and/or
        /// a /Differences array).
        bool m_has_unicode_map{ false };
        /// Whether the font's character codes are UTF-16BE code units (i.e., its
        /// /Encoding is one of Adobe's predefined Unicode CMaps, such as
        /// /UniJIS-UCS2-H), making the codes directly convertible to text even
        /// with no ToUnicode CMap.
        bool m_codes_are_utf16{ false };
        /// Whether this is a (notorious) symbol font (e.g., Symbol, Wingdings, ZapfDingbats).
        bool m_symbol_font{ false };
        /// Whether this font lays out text in vertical writing mode (its /Encoding
        /// is one of Adobe's predefined "-V" CMaps, such as /Identity-V or
        /// /UniJIS-UCS2-V).
        bool m_vertical_writing_mode{ false };
        };

    /** @brief Function object interface used to decompress a (zlib/DEFLATE
            compressed) stream section.
        @details The functor is passed the raw compressed bytes of a stream object
            and should return the uncompressed bytes (as a @c std::string, since the
            uncompressed content is still 8-bit PDF page-description syntax that this
            parser will then extract the text from).
        @returns An empty string upon failure.*/
    using stream_decompress_functor = std::function<std::string(std::string_view)>;

    /// @brief Catalog of a PDF file's indirect objects, along with the state
    ///     needed to resolve references and decode streams.
    class pdf_document
        {
      public:
        /// @brief Constructs a document catalog from raw PDF file content.
        /// @param fileContent The raw bytes of the entire PDF file.
        /// @param decompressor Functor used to decompress @c FlateDecode streams.
        /// @param logFunction Callback invoked with warning/error messages.
        /// @param glyphTable An optional glyph name table (e.g., the Adobe Glyph List) for
        ///     resolving simple fonts' `/Differences` custom encodings. May be @c nullptr
        ///     (or empty) if not loaded, in which case `/Differences` entries are ignored.
        /// @param cidTables An optional registry of CID-to-Unicode tables for resolving
        ///     Type0/CID fonts that have no embedded ToUnicode CMap. May be @c nullptr
        ///     (or empty) if none were loaded, in which case such fonts' text is dropped.
        pdf_document(std::string_view fileContent, const stream_decompress_functor& decompressor,
                     std::function<void(const std::wstring&)> logFunction,
                     const glyph_name_table* glyphTable = nullptr,
                     const cid_to_unicode_registry* cidTables = nullptr);

        /// @brief Scans the file for indirect objects (`N G obj ... endobj`).
        /// @details Also reads the file's cross-reference table(s) (classic
        ///     `xref` tables and `/Type /XRef` streams) and drops any object
        ///     whose most recent revision marks it free. Without this, an
        ///     object deleted by an incremental update would still be found
        ///     and used, since its (now-stale) body remains physically
        ///     present earlier in the file.
        void catalog_objects();

        /// @brief Expands compressed object streams (`/Type /ObjStm`, PDF 1.5+),
        ///     cataloging the objects packed inside of them.
        void expand_object_streams();

        /// @returns The object with the given number, or @c nullptr if not found.
        [[nodiscard]]
        const pdf_object* find_object(long objectNumber) const;

        /// @returns The object that a (possibly indirect reference) value points to,
        ///     or @c nullptr if the value is not a reference or is a dangling one.
        [[nodiscard]]
        const pdf_object* resolve_to_object(std::string_view value) const;

        /// @brief Follows indirect references until reaching a direct value.
        /// @details Real PDFs chain indirect references at most 2-3 levels deep;
        ///     the limit of 8 is a safeguard against circular references in
        ///     malformed files.
        /// @returns The direct value (a referenced object's dictionary or value).
        [[nodiscard]]
        std::string_view resolve_value(std::string_view value) const;

        /// @brief Runs a stream object's payload through its filter chain.
        /// @returns The decoded stream content (empty upon failure).
        [[nodiscard]]
        std::string decode_stream(const pdf_object& streamObject) const;

        /// @brief Sets the charset converter used to decode Type0 fonts whose
        ///     /Encoding is one of Adobe's predefined CJK CMaps (e.g., /ETenms-B5-H).
        /// @details Must be called before any fonts are loaded (i.e., before parsing
        ///     page content), since the converter is copied into each font's decoder
        ///     as the font is loaded.
        /// @param convertFunction The charset conversion functor to use.
        /// @sa charset_convert_functor.
        void set_charset_converter(charset_convert_functor convertFunction)
            {
            m_charset_convert = std::move(convertFunction);
            }

        /// @brief Sets the decryptor to use for this document (for PDFs that use the
        ///     standard security handler with an empty user password).
        /// @details Must be called (if at all) after catalog_objects() but before
        ///     expand_object_streams() and any other use of decode_stream() or
        ///     decrypt(), since encrypted object streams must be decrypted before
        ///     they can be expanded.
        /// @param decryptor The decryptor to use, or @c nullptr if the document
        ///     isn't encrypted. Must outlive this @c pdf_document.
        void set_decryptor(const pdf_decryptor* decryptor) noexcept { m_decryptor = decryptor; }

        /// @brief Decrypts a string or stream's raw bytes, if this document is
        ///     encrypted.
        /// @param owningObject The indirect object that directly contains @c bytes.
        /// @param bytes The (possibly encrypted) raw bytes.
        /// @returns @c bytes decrypted, or unchanged if this document isn't
        ///     encrypted, or if @c owningObject was unpacked from a compressed
        ///     object stream (in which case it's already plaintext, having been
        ///     decrypted as part of that stream as a whole).
        [[nodiscard]]
        std::string decrypt(const pdf_object& owningObject, std::string_view bytes) const;

        /// @brief Builds (or fetches from cache) the decoder for a font object.
        /// @returns The font decoder, or a default-constructed one if not found.
        [[nodiscard]]
        std::shared_ptr<pdf_font_decoder> load_font(long objectNumber);

        /// @brief Builds the decoder for a font from its dictionary.
        /// @returns A new font decoder populated from the dictionary entries.
        [[nodiscard]]
        std::shared_ptr<pdf_font_decoder>
        load_font_from_dictionary(std::string_view fontDictionary);

        /// @returns The order in which objects were found in the file.
        [[nodiscard]]
        const std::vector<long>& get_scan_order() const noexcept;

      private:
        /// @brief Decodes an ASCIIHexDecode filtered stream.
        [[nodiscard]]
        static std::string ascii_hex_decode(std::string_view data);

        /// @brief Decodes an ASCII85Decode filtered stream.
        [[nodiscard]]
        static std::string ascii85_decode(std::string_view data);

        /// @brief Decodes an LZWDecode filtered stream (the PDF/TIFF variant of LZW,
        ///     using the "early change" convention for widening its codes).
        /// @returns The decoded bytes, or an empty string if the data uses the
        ///     (rare, unsupported) alternate LZW flavor.
        [[nodiscard]]
        static std::string lzw_decode(std::string_view data);

        /// @brief Reverses a PNG predictor that was applied before compression
        ///     (used by some object and cross-reference streams).
        [[nodiscard]]
        static std::string apply_png_predictor(const std::string& data, size_t columns,
                                               size_t colors, size_t bitsPerComponent);

        /// @brief Reads the file's cross-reference table(s) and drops any
        ///     object from @c m_objects whose most recent revision marks it free.
        /// @details Called at the end of catalog_objects(). Reads classic
        ///     `xref` tables directly out of @c m_file, then cross-reference
        ///     streams (already cataloged as ordinary objects by that point)
        ///     via apply_xref_stream_free_entries(). A later xref record for
        ///     a given object number overwrites an earlier one, the same
        ///     "later occurrence wins" rule catalog_objects() already applies
        ///     to object bodies.
        void exclude_free_objects();

        /// @brief Reads the free/in-use status of each entry in a
        ///     cross-reference stream (`/Type /XRef`) and records it in
        ///     @c freeStatus.
        /// @param xrefObject The cross-reference stream object.
        /// @param freeStatus Map of object number to free status, updated
        ///     in place (entries for object numbers not covered by this
        ///     stream's `/Index` are left untouched).
        void apply_xref_stream_free_entries(const pdf_object& xrefObject,
                                            std::map<long, bool>& freeStatus) const;

        std::string_view m_file;
        stream_decompress_functor m_decompress;
        charset_convert_functor m_charset_convert;
        std::function<void(const std::wstring&)> m_log;
        const glyph_name_table* m_glyph_name_table{ nullptr };
        const cid_to_unicode_registry* m_cid_to_unicode_tables{ nullptr };
        const pdf_decryptor* m_decryptor{ nullptr };
        std::map<long, pdf_object> m_objects;
        std::vector<long> m_object_scan_order;
        // owns decompressed object-stream data; uses a deque (not a vector) so that
        // push_back never invalidates existing string_views into previously-pushed strings
        std::deque<std::string> m_decompressed_buffers;
        std::map<long, std::shared_ptr<pdf_font_decoder>> m_font_cache;
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // PDF_DOCUMENT_H
