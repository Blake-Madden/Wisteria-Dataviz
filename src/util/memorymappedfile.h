/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef MEMMAPPEDFILE_H
#define MEMMAPPEDFILE_H

#if defined(__LINUX__) || defined(__APPLE__) || defined(__BSD__) || defined(__WXGTK__)
    #include <sys/fcntl.h>
    #include <sys/mman.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

#include <exception>
#include <stdexcept>
#include <wx/file.h>
#include <wx/log.h>
#include <wx/longlong.h>
#include <wx/msgdlg.h>
#include <wx/string.h>

/// @brief General exception that can be thrown when mapping a file.
class MemoryMappedFileException final : public std::exception
    {
    };

/// @brief Exception that can be thrown when mapping if the file is zero length.
class MemoryMappedFileEmptyException final : public std::exception
    {
    };

/// @brief Exception that can be thrown when mapping if the file can't be exclusively locked.
class MemoryMappedFileShareViolationException final : public std::exception
    {
    };

/// @brief Exception that can be thrown when mapping if the file isn't something that can be mapped.
class MemoryMappedInvalidFileType final : public std::exception
    {
    };

/// @brief Exception that can be thrown when mapping if the size of the file can't be determined.
class MemoryMappedInvalidFileSize final : public std::exception
    {
    };

/// @brief Exception with reading file in the cloud (web service).
class MemoryMappedFileCloudFileError final : public std::exception
    {
    };

#ifdef __WXMSW__
/// @private
using MemoryMappedFileHandleType = HANDLE;
#else
/// @private
using MemoryMappedFileHandleType = int;
#endif

/**
@brief Class for mapping a file into your address space
    (rather than having to buffer its contents).
@par Example
@code
    try
        {
        MemoryMappedFile fileMap(L"/home/blake/file.txt", true);
        const char* fileText = static_cast<const char*>(fileMap.GetStream());
        // now map another file (note that fileText will not be valid after this)
        fileMap.UnmapFile();
        fileMap.MapFile(L"/home/bmadden/DifferentFile.txt", false);
        char* writableFileText = static_cast<char*>(fileMap.GetStream());
        // now write back to the file by writing to the pointer
        std::strncpy(writableFileText, "Hello, world!", 13);
        }
    catch (...)
        {
        // handle error here
        }
@endcode
@todo Currently only supports files under 2GBs.
*/
class MemoryMappedFile
    {
  public:
    /// @brief Default Constructor.
    MemoryMappedFile() noexcept
        :
#ifdef __WXMSW__
          m_hFile(INVALID_HANDLE_VALUE), m_hsection(NULL),
#else
          m_hFile(-1),
#endif
          m_data(nullptr), m_bufferedData(nullptr), m_mapSize(0), m_open(false), m_isReadOnly(true),
          m_isBuffered(false)
        {
        }

    /** @brief Constructor which will automatically map the file.

        @exception MemoryMappedFileException
        @exception MemoryMappedFileEmptyException
        @exception MemoryMappedFileShareViolationException
        @exception MemoryMappedInvalidFileType
        @exception MemoryMappedInvalidFileSize
        @exception MemoryMappedFileCloudFileError

        @param filePath Path to the file to map.
        @param readOnly Flag specifying whether to open the file as read only.
        @param autoBufferOnException Flag specifying whether to attempt to buffer
            the file if the mapping fails. An exception may still be thrown if the
            buffering fails. Note that this is not recommended for large files
            as the buffer will be as large as the file.*/
    explicit MemoryMappedFile(const wxString& filePath, bool readOnly = true,
                              const bool autoBufferOnException = false)
        :
#ifdef __WXMSW__
          m_hFile(INVALID_HANDLE_VALUE), m_hsection(NULL),
#else
          m_hFile(-1),
#endif
          m_data(nullptr), m_bufferedData(nullptr), m_mapSize(0), m_open(false),
          m_isReadOnly(readOnly), m_isBuffered(false)
        {
        MapFile(filePath, readOnly, autoBufferOnException);
        }
    /// @private
    MemoryMappedFile(const MemoryMappedFile&) = delete;
    /// @private
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;

    /// @brief Destructor which implicitly unmaps the file.
    ~MemoryMappedFile() { UnmapFile(); }

    /// @returns Whether a file is currently (and successfully) mapped.
    [[nodiscard]]
    bool IsOk() const noexcept
        {
        return m_open;
        }

    /// @returns Whether the current file mapping is read only.
    [[nodiscard]]
    bool IsReadOnly() const noexcept
        {
        return m_isReadOnly;
        }

    /** @brief Manually maps a new file.
        @warning If this object is currently mapping another file then
        you need to call UnmapFile() first.

        @exception MemoryMappedFileException
        @exception MemoryMappedFileEmptyException
        @exception MemoryMappedFileShareViolationException
        @exception MemoryMappedInvalidFileType
        @exception MemoryMappedInvalidFileSize
        @exception MemoryMappedFileCloudFileError

        @param filePath Path to the file to map.
        @param readOnly Flag specifying whether to open the file as read only.
        @param autoBufferOnException Flag specifying whether to attempt to buffer
            the file if the mapping fails. An exception may still be thrown if
            the buffering fails. Note that this is not recommended for large files
            as the buffer will be as large as the file.

        @returns @c true if file mapping (or buffering) was successful.*/
    bool MapFile(const wxString& filePath, bool readOnly = true,
                 bool autoBufferOnException = false);
    /// @brief Closes the handles and mappings.
    void UnmapFile();

    /** @returns The raw byte stream of the file.
        @warning Do not attempt to write to the returned pointer if you mapped the file as
            read only. The read only status of the current mapping can be checked by
            calling IsReadOnly().*/
    [[nodiscard]]
    void* GetStream()
        {
        return IsBuffered() ? reinterpret_cast<void*>(m_bufferedData) : m_data;
        }

    /** @returns The raw byte stream of the file.*/
    [[nodiscard]]
    const void* GetStream() const
        {
        return IsBuffered() ? reinterpret_cast<const void*>(m_bufferedData) : m_data;
        }

    /// @returns The length of the mapped file.
    [[nodiscard]]
    size_t GetMapSize() const noexcept
        {
        return m_mapSize;
        }

    /// @returns The path of the file currently mapped.
    [[nodiscard]]
    const wxString& GetFilePath() const noexcept
        {
        return m_filePath;
        }

    /// @returns Whether the mapping had failed and the file had to be buffered instead.
    [[nodiscard]]
    bool IsBuffered() const noexcept
        {
        return m_isBuffered;
        }

    /// @returns The size of a large file (as an unsigned long long).
    /// @param hFile The file's handle.
    [[nodiscard]]
    static wxULongLong GetFileSize64(MemoryMappedFileHandleType hFile);

  private:
    void Reset(bool preserveFileName = false);
    bool Buffer();
#ifdef __WXMSW__
    HANDLE m_hFile{ INVALID_HANDLE_VALUE };
    HANDLE m_hsection{ NULL };
#else
    int m_hFile{ -1 };
#endif
    void* m_data{ nullptr };
    char* m_bufferedData{ nullptr };
    size_t m_mapSize{ 0 };
    wxString m_filePath;
    bool m_open{ false };
    bool m_isReadOnly{ true };
    bool m_isBuffered{ false };
    };

    /** @}*/

#endif // MEMMAPPEDFILE_H
