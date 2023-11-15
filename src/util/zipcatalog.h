/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WXZIPHELPER_H__
#define __WXZIPHELPER_H__

#include "textstream.h"
#include <map>
#include <vector>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/stdpaths.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/zstream.h>

namespace Wisteria
    {
    /** @brief Stores error and warning messages encountered while loading an archive.*/
    struct ArchiveMessage
        {
        /// @brief Constructor.
        /// @param message The warning/error message.
        /// @param icon The icon to show (if presented in a message box).
        ArchiveMessage(const wxString& message, const int icon) :
            m_message(message), m_icon(icon)
            {}
        /// The warning/error message.
        wxString m_message;
        /// The icon to show (if presented in a message box).
        int m_icon{ 0 };
        };

    /// @brief Helper class that reads files inside of a zip file by just passing in the file path.
    class ZipCatalog
        {
    public:
        /// @private
        ZipCatalog() = default;
        /** @brief Loads a ZIP file stream and catalogs its files.
            @param data The (`char*`) file stream to load.
            @param len The length of the file stream.*/
        ZipCatalog(const void* data, size_t len);
        /** @brief Constructor.
            @param zipFilePath The path to the ZIP file to open.*/
        explicit ZipCatalog(const wxString& zipFilePath);
        /// @private
        ZipCatalog(const ZipCatalog&) = delete;
        /// @private
        ZipCatalog& operator=(const ZipCatalog&) = delete;
        /// @private
        ~ZipCatalog();

        /** @brief Loads a ZIP file stream and catalogs its files.
            @param data The (char*) file stream to load.
            @param len The length of the file stream.*/
        void Init(const void* data, size_t len);

        /** @brief Searches for an entry in the ZIP file by name.
            @param value The name of the file to search for.
            @returns The file entry in the ZIP file or null if not found.*/
        [[nodiscard]]
        wxZipEntry* Find(const wxString& value) const
            {
            auto entry = m_catalog.find(wxZipEntry::GetInternalName(value));
            return (entry == m_catalog.cend() ) ? nullptr : entry->second;
            }
        /** @returns A list of files in a given folder
                (relative to its location in the ZIP folder structure).
            @param path The name of the folder to iterate.*/
        [[nodiscard]]
        wxArrayString GetFilesInFolder(const wxString& path) const;
        /** @returns A list of all files (relative to its location in the ZIP folder structure).*/
        [[nodiscard]]
        wxArrayString GetPaths() const;
        /** @brief Reads a text file in the archive and returns its content.
            @details File will be converted to unicode, so this should only be called on actual text
                files (e.g., *.txt, *.htm, etc.).
            @param path The path to the file (relative to its location in the ZIP folder structure).
            @returns The text file's content.*/
        [[nodiscard]]
        wxString ReadTextFile(const wxString& path) const;
        /** @brief Reads a file in the ZIP file and returns its content in a `char*` memory stream.
            @param path The path to the file (relative to its location in the ZIP folder structure).
            @param memstream The memory stream (which will be a `char*` buffer) to write to.
            @returns @c true if successfully read the file.*/
        bool ReadFile(const wxString& path, wxOutputStream& memstream) const;
        /** @brief Reads an image from the ZIP file.
            @param path The path to the image (relative to its location in the ZIP folder structure).
            @param bitmapType The format of the image.
            @returns The bitmap from the ZIP file, or @c wxNullBitmap if not found.*/
        [[nodiscard]]
        wxBitmap ReadBitmap(const wxString& path, const wxBitmapType bitmapType) const;
        /** @brief Reads an SVG entry from the ZIP file.
            @param path The path to the image (relative to its location in the ZIP folder structure).
            @param size The default size for the SVG.
            @returns The SVG as a @c wxBitmap from the ZIP file
                (which will be invalid if the path was not found).*/
        [[nodiscard]]
        wxBitmap ReadSVG(const wxString& path, const wxSize size) const;
        /**@brief Reads a text file in the archive, converts its contents to Unicode text,
                and copies it to a temp file.
           @param path The path to the file (relative to its location in the ZIP folder structure).
           @returns The path to the temp file holding the file's contents. Note that the caller is
               responsible for deleting this file when finished with it.*/
        [[nodiscard]]
        wxString ExtractTextFileToTempFile(const wxString& path) const;

        /// @brief Writes a string to an output buffer as UTF-8, simplifying all conversions.
        /// @param zip The zip buffer to write to.
        /// @param fileName The name of the file to write this text as into.
        /// @param text The text to write to the stream.
        static void WriteText(wxZipOutputStream& zip, const wxString& fileName,
                              const wxString& text);
        /// @returns @c true if any read operations had failed,
        ///     usually due to corruption or password protection.
        [[nodiscard]]
        bool HadReadErrors() const noexcept
            { return m_readErrorShown; }
        /// @returns The messages encountered while loading files from the archive.
        [[nodiscard]]
        const std::vector<ArchiveMessage>& GetMessages() const noexcept
            { return m_messages; }
        /// @brief Clears all of the logged messages from previous reads.
        void ClearMessages() noexcept
            { m_messages.clear(); }
    private:
        /// @brief Optimized reading function which can use a
        ///     large buffer for larger compressed files.
        bool Read(wxInputStream* stream_in, wxOutputStream& stream_out,
                  const size_t bufferSize) const;
        wxZipInputStream* m_inzip{ nullptr };
        std::map<wxString, wxZipEntry*> m_catalog;
        MemoryMappedFile m_mapfile;
        // bookkeeping
        mutable std::vector<ArchiveMessage> m_messages;
        mutable bool m_readErrorShown{ false };
        };
    }

/** @}*/

#endif //__WXZIPHELPER_H__
