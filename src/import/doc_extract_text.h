/** @addtogroup Importing
    @brief Classes for importing and parsing text.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef DOC_EXTRACTOR_H
#define DOC_EXTRACTOR_H

#include "extract_text.h"
#include <array>
#include <utility>
#include <vector>

namespace lily_of_the_valley
    {
    // clang-format off
    /** @brief Class to extract text from a Microsoft® Word 97-2003 file.
        @details This file format is the Compound File Binary File Format (aka OLE 2.0):

        https://poi.apache.org/components/poifs/index.html <br />
        https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-cfb/53989ce4-7b05-4f8d-829b-d08d6148375b <br />
        http://download.microsoft.com/download/0/B/E/0BE8BDD7-E5E8-422A-ABFD-4342ED7AD886/Word97-2007BinaryFileFormat%28doc%29Specification.pdf <br />
        https://github.com/libyal/libolecf/blob/master/documentation/OLE%20Compound%20File%20format.asciidoc

        File Information Block (FIB) flags:

        https://docs.microsoft.com/en-us/openspecs/office_file_formats/ms-doc/26fb6c06-4e5c-4778-ab4e-edbf26a545bb

        Properties (SummaryInformation entry):

        https://poi.apache.org/components/hpsf/internals.html <br />
        https://docs.microsoft.com/en-us/windows/win32/stg/the-summary-information-property-set
    */
    // clang-format on

    class word1997_extract_text final : public extract_text
        {
      public:
        /// @brief Malformed BAT in document.
        class cfb_bad_bat : public std::exception
            {
            };

        /// @brief Malformed BAT entry in document.
        class cfb_bad_bat_entry : public std::exception
            {
            };

        /// @brief Malformed XBAT in document.
        class cfb_bad_xbat : public std::exception
            {
            };

        /// @brief Malformed XBAT entry in document.
        class cfb_bad_xbat_entry : public std::exception
            {
            };

        /// @brief Encrypted document.
        class msword_encrypted : public std::exception
            {
            };

        /// @brief Corrupted document.
        class msword_corrupted : public std::exception
            {
            };

        /// @brief Fast-saved document.
        class msword_fastsaved : public std::exception
            {
            };

        /// @brief Missing header section missing in document.
        class msword_header_not_found : public std::exception
            {
            };

        /// @brief Root entry object missing in document.
        class msword_root_entry_not_found : public std::exception
            {
            };

        /// @private
        word1997_extract_text() = default;
        /// @private
        word1997_extract_text(const word1997_extract_text& that) = delete;
        /// @private
        word1997_extract_text& operator=(const word1997_extract_text& that) = delete;

        /** @brief Main interface for extracting plain text from a DOC buffer.
            @param doc_buffer The DOC file text to load.
            @param text_length The length of the text.
            @returns The filtered (i.e., the plain text) from the stream.*/
        const wchar_t* operator()(const char* const doc_buffer, const size_t text_length);

        /** @returns The title from the document summary.
            @note Must be called after calling @c operator().*/
        [[nodiscard]]
        const std::wstring& get_title() const noexcept
            {
            return m_title;
            }

        /** @returns The subject from the document summary.
            @note Must be called after calling @c operator().*/
        [[nodiscard]]
        const std::wstring& get_subject() const noexcept
            {
            return m_subject;
            }

        /** @returns The author from the document summary.
            @note Must be called after calling @c operator().*/
        [[nodiscard]]
        const std::wstring& get_author() const noexcept
            {
            return m_author;
            }

        /** @returns The keywords from the document summary.
            @note Must be called after calling @c operator().*/
        [[nodiscard]]
        const std::wstring& get_keywords() const noexcept
            {
            return m_keywords;
            }

        /** @returns The comments from the document summary.
            @note Must be called after calling @c operator().*/
        [[nodiscard]]
        const std::wstring& get_comments() const noexcept
            {
            return m_comments;
            }

// give unit testing full access to this class
#ifndef __UNITTEST
      private:
#endif
        /// @brief Character set of text streams.
        enum class charset_type
            {
            mbcs,        /*!< Multi-byte Character Set */
            utf16,       /*!< UTF-16 */
            type_unknown /*!< Unknown */
            };

        /// @brief Types of entries in the file system.
        enum class file_system_entry_type
            {
            unknown_unallocated_type = 0x00, /*!< garbage */
            storage_type = 0x01,             /*!< "folder" */
            stream_type = 0x02,              /*!< "file" stream */
            // MS docs don't mention anything for values 0x03 and 0x04,
            // but others' research mentions "lock" and "property"?
            // I have not encountered these values in the wild though.
            root_storage_type = 0x05 /*!< root */
            };

        /// @brief Entry node color.
        enum class file_system_entry_color
            {
            red = 0,  /*!< red node */
            black = 1 /*!< black node */
            };

        /// @brief Variant types that a property (from the SummaryInformation) can be.
        enum class property_data_type
            {
            vt_bstr = 8,     /*!< COM string */
            vt_lpstr = 30,   /*!< char* string */
            vt_lpwstr = 31,  /*!< wchar_t* string */
            vt_filetime = 64 /*!< time */
            };

        /// @brief Property format IDs.
        enum class property_format_id
            {
            pid_title = 0x02,    /*!< title */
            pid_subject = 0x03,  /*!< subject */
            pid_author = 0x04,   /*!< author */
            pid_keywords = 0x05, /*!< keyword */
            pid_comments = 0x06  /*!< comments */
            };

        /// @brief Stream interface used to read data from a file system entry.
        class cfb_iostream
            {
          public:
            /// @brief Methods for repositioning within a cfb_iostream stream.
            enum class cfb_stream_seek_type
                {
                seek_beg = 0, /*!< stream beginning */
                seek_cur = 1, /*!< stream's current position */
                seek_end = 2  /*!< stream end */
                };

            /// @private
            cfb_iostream(const char* start, const size_t size) noexcept
                : m_start(start), m_current_position(start), m_buffer_size(size),
                  m_eof(start + size)
                {
                }

            /// @private
            cfb_iostream() = delete;

            /// @private
            cfb_iostream(const cfb_iostream&) noexcept = default;

            /// @private
            cfb_iostream& operator=(const cfb_iostream&) = delete;

            /// @private
            virtual ~cfb_iostream() = default;

            /// @returns @c true if current position is at the end.
            [[nodiscard]]
            virtual bool eof() const noexcept
                {
                return m_current_position == m_eof;
                }

            /// @returns How far we have moved into the stream.
            [[nodiscard]]
            size_t tell() const noexcept
                {
                return (m_current_position - m_start);
                }

            /** @brief Moves the stream cursor.
                @param offset How far to move the cursor.
                @param origin Where to start moving from.
                @returns How far we have moved into the stream.*/
            size_t seek(const long offset, const cfb_stream_seek_type origin)
                {
                if (origin == cfb_stream_seek_type::seek_cur)
                    {
                    // if going backwards too far, then move to the start
                    if ((static_cast<long>(tell()) + offset) < 0)
                        {
                        m_current_position = m_start;
                        }
                    // if offset is going too far, then move to the end
                    else if (tell() + offset > m_buffer_size)
                        {
                        m_current_position = m_eof;
                        }
                    else
                        {
                        m_current_position += offset;
                        }
                    }
                else if (origin == cfb_stream_seek_type::seek_beg)
                    {
                    // negative positive is bogus, so move to start of file
                    if (offset < 0)
                        {
                        m_current_position = m_start;
                        }
                    // if offset is going too far, then move to the end
                    else if (std::cmp_greater(offset, m_buffer_size))
                        {
                        m_current_position = m_eof;
                        }
                    else
                        {
                        m_current_position = m_start + offset;
                        }
                    }
                else if (origin == cfb_stream_seek_type::seek_end)
                    {
                    // don't go beyond the end
                    if (offset > 0)
                        {
                        m_current_position = m_eof;
                        }
                    // don't go beyond the start (if going backwards)
                    else if (std::cmp_greater(std::abs(offset), m_buffer_size))
                        {
                        m_current_position = m_start;
                        }
                    else
                        {
                        m_current_position = m_eof + offset;
                        }
                    }
                return (m_current_position - m_start);
                }

            /** @brief Reads from the stream into a @c buffer.
                @param[out] buffer The buffer to be written to.
                @param size The number of bytes in the @c buffer.
                @returns The number of bytes read.
                @note This will move the current position in the stream.*/
            size_t read(void* buffer, const size_t size) noexcept
                {
                if (buffer == nullptr)
                    {
                    return 0;
                    }
                // verify that the current pos is safe to read from (because of a bad seek)
                if (m_current_position >= m_eof)
                    {
                    return 0;
                    }
                // check to see if the requested read size is bigger than the rest of the stream
                const size_t readSize = std::cmp_greater_equal(m_eof - m_current_position, size) ?
                                            size :
                                            m_eof - m_current_position;
                if (readSize == 1)
                    {
                    static_cast<char*>(buffer)[0] = m_current_position[0];
                    }
                else
                    {
                    std::memcpy(buffer, m_current_position, readSize);
                    }
                m_current_position += readSize;
                return readSize;
                }

            /// @returns The beginning of the file stream.
            [[nodiscard]]
            const char* get_start() const noexcept
                {
                return m_start;
                }

          private:
            const char* m_start{ nullptr };
            const char* m_current_position{ nullptr };
            size_t m_buffer_size{ 0 };
            const char* m_eof{ nullptr };
            };

        /// @brief A file system entry pointing to a "file."
        /// @details Usually, a storage (directory) or stream (file).
        class file_system_entry final : public cfb_iostream
            {
          public:
            /// @brief Constructor.
            /// @param str The file stream to enter.
            explicit file_system_entry(const cfb_iostream& str) noexcept : cfb_iostream(str) {}

            /// @private
            file_system_entry() = delete;
            /// @private
            file_system_entry(const file_system_entry&) = delete;
            /// @private
            file_system_entry& operator=(const file_system_entry&) = delete;

            /** @brief Opens stream, which corresponds to last-read directory object.
                @returns @c 0 on success, @c -1 if not a valid CFB stream.*/
            [[nodiscard]]
            int open() noexcept
                {
                if (m_type != file_system_entry_type::stream_type)
                    {
                    return -1;
                    }

                m_internal_offset = 0;
                m_stream_offset = tell();
                return 0;
                }

            /// @returns @c true if this is a root storage or the main storage area.
            /// @note The name "Root Entry" is not reliable in older files
            ///     (that would just say "R"), so the type is also checked.
            [[nodiscard]]
            bool is_root_entry() const noexcept
                {
                return (m_type == file_system_entry_type::root_storage_type ||
                        m_name == ROOT_ENTRY);
                }

            /// @returns @c true if cursor is at the end of the object.
            [[nodiscard]]
            bool eof() const noexcept final
                {
                return (m_internal_offset >= m_size);
                }

            /// @returns @c true if property is stored in SBAT (and not the BAT or XBATs).\n
            ///     This only happens with smaller files.
            [[nodiscard]]
            bool is_in_small_blocks() const noexcept
                {
                return (m_size < 4096) && !is_root_entry();
                }

            static const std::string ROOT_ENTRY;

            std::string m_name;
            size_t m_size{ 0 };               /// size of the object
            int32_t m_previous_property{ 0 }; /// previous property in table
            int32_t m_next_property{ 0 };     /// previous property in table
            size_t m_internal_offset{ 0 };    /// offset into object's own structure
            size_t m_stream_offset{ 0 };      /// offset into the parent stream
            const char* m_storage_offset{ nullptr };
            file_system_entry_type m_type{ file_system_entry_type::unknown_unallocated_type };
            file_system_entry_color m_color{ file_system_entry_color::black };
            std::vector<long int> m_sectors; /// where the data actually is
            };

        /// Keeps track of the current state while parsing a stream.
        struct parse_state
            {
            bool m_hyperlink_begin_char_detected{ false };
            bool m_hyperlink_is_valid{ false };
            bool m_non_printable_char_detected{ false };
            bool m_force_output_write{ false };
            bool m_is_in_table{ false };
            bool m_consecutive_table_tabs_detected{ false };
            bool m_at_start_of_new_block{ false };
            };

        /// @brief Loads the contents of a file system stream and writes the
        ///     text paragraphs into the output buffer.
        /// @param cfbObj The stream entry to load.
        void load_stream(file_system_entry* cfbObj);

        /// @returns @c true if a @c paragraph ends with a newline.
        [[nodiscard]]
        static bool paragraph_ends_with_crlf(const std::wstring& paragraph)
            {
            if (paragraph.empty())
                {
                return false;
                }
            return (paragraph.back() == 13 || paragraph.back() == 10);
            }

        /** @brief Loads text body section (i.e., the "WordDocument" section).
            @param cfbObj The "WordDocument" storages entry to load.*/
        void load_document(file_system_entry* cfbObj);

        /** @brief Loads document properties (i.e., the "\005SummaryInformation" section).
            @param cfbObj The "\005SummaryInformation" storages entry to load.*/
        void load_summary_information(file_system_entry* cfbObj);

        /// @returns Whether a paragraph begins with a specified string.\n
        ///     Leading spaces will be skipped.
        [[nodiscard]]
        static bool paragraph_begins_with(const std::wstring& para, std::wstring_view searchText)
            {
            if (searchText.empty())
                {
                return false;
                }
            const size_t firstChar = para.find_first_not_of(L" \n\r\t");
            if (firstChar == std::string_view::npos)
                {
                return false;
                }
            return (std::wcsncmp(para.c_str() + firstChar, searchText.data(),
                                 searchText.length()) == 0);
            }

        /** @brief Scans a sector to determine if it is really a binary stream,
                such as an embedded image.
            @details Usually, embedded CompObjs sectors are not mixed in with the text sectors,
                but sometimes they are in messed up files, so we need to watch out for these
                and skip them if we can.
            @param buffer The buffer to review.
            @param size The length of the buffer.
            @returns @c true if a binary stream.*/
        [[nodiscard]]
        static bool is_buffer_binary_stream(const unsigned char* buffer,
                                            const size_t size) noexcept;

        /* @brief Loads the header of the document.
           @param str A CFB stream (i.e., the Word file).
           @returns @c true if a valid header that was successfully loaded.*/
        [[nodiscard]]
        bool load_header(cfb_iostream* str);

        /* @brief Loads a property (usually a storage directory),
               based on where we are currently in the Entry Table.
           @param str The document stream to read from.
           @returns file_system_entry The property object.*/
        [[nodiscard]]
        std::shared_ptr<file_system_entry> read_next_file_system_entry(const cfb_iostream* str);

        /** @brief Reads text (i.e., a "file" in the "file system") from a property.
            @param[out] buffer The pointer to the buffer to write into.
            @param[in,out] bufferSize The size of the buffer (will change if buffer is resized).
            @param cfbObj The entry to read from
                (will have its file position moved during the read).
            @returns The number of bytes read.*/
        size_t read_stream(void* buffer, size_t bufferSize, file_system_entry* cfbObj) const;

        /// @brief General cleanup.
        void reset() noexcept
            {
            m_file_end_sentinel = m_file_system_entries = m_current_file_system_entry = nullptr;
            m_BAT.clear();
            m_SBAT.clear();
            // reset doc meta data
            m_title.clear();
            m_subject.clear();
            m_author.clear();
            m_keywords.clear();
            m_comments.clear();
            }

        // endian conversions, referred to in MS specification.
        //-----------------------------------------------------

        /// @returns A 32-bit unsigned int from @c buffer at given @c offset.
        [[nodiscard]]
        static /*DWORD*/ uint32_t read_uint(const char* buffer, const size_t offset) noexcept
            {
            if (buffer == nullptr)
                {
                return 0;
                }
            return static_cast<uint32_t>(static_cast<unsigned char>(buffer[offset])) |
                   (static_cast<uint32_t>(static_cast<unsigned char>(buffer[offset + 1])) << 8L) |
                   (static_cast<uint32_t>(static_cast<unsigned char>(buffer[offset + 2])) << 16L) |
                   (static_cast<uint32_t>(static_cast<unsigned char>(buffer[offset + 3])) << 24L);
            }

        /// @returns A 32-bit signed int from @c buffer at given @c offset.
        [[nodiscard]]
        static /*INT*/ int32_t read_int(const char* buffer, const size_t offset) noexcept
            {
            if (buffer == nullptr)
                {
                return 0;
                }
            return static_cast<int32_t>(static_cast<unsigned char>(buffer[offset])) |
                   (static_cast<int32_t>(static_cast<unsigned char>(buffer[offset + 1])) << 8L) |
                   (static_cast<int32_t>(static_cast<unsigned char>(buffer[offset + 2])) << 16L) |
                   (static_cast<int32_t>(static_cast<unsigned char>(buffer[offset + 3])) << 24L);
            }

        /// @returns A 16-bit unsigned integer from @c buffer at given @c offset.
        [[nodiscard]]
        static /*SHORT*/ uint16_t read_short(const char* buffer, const int offset) noexcept
            {
            if (buffer == nullptr)
                {
                return 0;
                }
            return static_cast<uint16_t>(static_cast<unsigned char>(buffer[offset])) |
                   (static_cast<uint16_t>(static_cast<unsigned char>(buffer[offset + 1])) << 8);
            }

        /// @returns A 16-bit unsigned integer from @c buffer at given @c offset.
        [[nodiscard]]
        static /*SHORT*/ uint16_t read_short(const unsigned char* buffer, const int offset) noexcept
            {
            if (buffer == nullptr)
                {
                return 0;
                }
            return static_cast<uint16_t>(buffer[offset]) |
                   (static_cast<uint16_t>(buffer[offset + 1]) << 8);
            }

        /// @brief Modulus operation that checks for modulus by zero or into zero
        ///     (returns zero for those situations).
        /// @param dividend The dividend (i.e., the value being divided).
        /// @param divisor The divisor (i.e., the value dividing by).
        /// @returns The remainder of the modulus operation, or zero if one
        ///     of the values was invalid.
        template<typename T>
        [[nodiscard]]
        inline constexpr static T safe_modulus(const T dividend, const T divisor) noexcept
            {
            if (dividend == 0 || divisor == 0)
                {
                return 0;
                }
            return dividend % divisor;
            }

        /// @brief Division operation that checks for division by zero or into zero
        ///     (returns zero for those situations).
        /// @param dividend The dividend (i.e., the value being divided).
        /// @param divisor The divisor (i.e., the value dividing by).
        /// @returns The quotient of the division operation,
        ///     or zero if one of the values was invalid.
        /// @note If the template type has floating point precision,
        ///     then the result will retain its precision.
        template<typename T>
        [[nodiscard]]
        inline constexpr static T safe_divide(const T dividend, const T divisor) noexcept
            {
            if (dividend == 0 || divisor == 0)
                {
                return 0;
                }
            return dividend / static_cast<T>(divisor);
            }

        /// @returns The number of SBATs that can fit in a sector.
        [[nodiscard]]
        size_t get_sbats_per_sector() const noexcept
            {
            return safe_divide(m_sector_size, m_short_sector_size);
            }

        /// @returns @c true if a stream starts with a Word header.
        inline bool starts_with_doc_header(const char* stream, const size_t len)
            {
            constexpr static std::array<unsigned char, 8> MAGIC_NUMBER = { 0xD0, 0xCF, 0x11, 0xE0,
                                                                           0xA1, 0xB1, 0x1A, 0xE1 };

            // Used in old beta versions of OLE2
            constexpr static std::array<unsigned char, 8> MAGIC_NUMBER_BETA = { 14, 17,   0xFC,
                                                                                13, 0xD0, 0xCF,
                                                                                17, 14 };

            return len >= MAGIC_NUMBER.size() &&
                   (std::memcmp(stream, MAGIC_NUMBER.data(), MAGIC_NUMBER.size()) == 0 ||
                    std::memcmp(stream, MAGIC_NUMBER_BETA.data(), MAGIC_NUMBER_BETA.size()) == 0);
            }

        // File Information Block (FIB) flags
        constexpr static int fComplex = 0x0004; /// Set when file is in complex, fast-saved format.
        constexpr static int fEncrypted = 0x0100; /// Set if file is encrypted.
        constexpr static int fExtChar = 0x1000;   /// Set when using extended character set in file.
        constexpr static int fFarEast =
            0x4000; /// Set when document was saved with CJK version of Word.

        // sector sizes
        constexpr static size_t SECTOR_SIZE = 256;
        constexpr static int BAT_SECTOR_SIZE = 512;
        constexpr static int SBAT_SECTOR_SIZE = 64;
        constexpr static int ENTRY_SECTOR_SIZE = 128;
        constexpr static int DIFAT_SIZE = 436;

        // file signatures
        static const std::string RTF_SIGNATURE;
        constexpr static std::array<uint8_t, 3> UTF8_SIGNATURE{ 0xEF, 0xBB, 0xBF };
        charset_type m_read_type{ charset_type::utf16 };

        size_t m_file_length{ 0 };
        size_t m_sector_count{ 0 };
        unsigned long m_text_body_stream_length{ 0 };

        std::shared_ptr<file_system_entry> m_root_storage;

        const char* m_file_end_sentinel{ nullptr };

        // Entry Table (the directory layout of the file system)
        size_t m_file_system_entry_count{ 0 };
        const char* m_file_system_entries{ nullptr };
        const char* m_current_file_system_entry{ nullptr };

        // Small BAT (aka Mini FAT) Info (text is stored here for small [> 4Kbs] files)
        size_t m_sbat_sector_count{ 0 };
        std::vector<char> m_SBAT;
        size_t m_short_sector_size{ SBAT_SECTOR_SIZE };

        // BAT (aka FAT) Info (used for medium-sized files)
        size_t m_bat_sector_count{ 0 };
        size_t m_sector_size{ BAT_SECTOR_SIZE };
        std::vector<char> m_BAT;

        // No data fields for XBATs here, but those are handled elsewhere (used for large files).
        // Each XBAT sector can add ~8Mbs to the file.

        // meta data
        std::wstring m_title;
        std::wstring m_subject;
        std::wstring m_author;
        std::wstring m_keywords;
        std::wstring m_comments;
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // DOC_EXTRACTOR_H
