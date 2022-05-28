///////////////////////////////////////////////////////////////////////////////
// Name:        memorymappedfile.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "memorymappedfile.h"

bool MemoryMappedFile::MapFile(const wxString& filePath,
                               const bool readOnly /*= true*/,
                               const bool autoBufferOnException /*= false*/)
    {
    if (filePath.IsEmpty())
        {
        wxLogError(L"Attempted to map a file with an empty file path.");
        return false;
        }
    //if another file is currently mapped this will fail
    if (IsOk())
        {
        wxLogWarning(L"Failed to map a file with another file map: %s", filePath);
        return false;
        }
    m_isReadOnly = readOnly;
    m_filePath = filePath;
#ifdef __WXMSW__
    unsigned long dwDesiredFileAccess = GENERIC_READ;
    if (!IsReadOnly())
        { dwDesiredFileAccess |= GENERIC_WRITE; }
    //get the handle to the file...
    m_hFile =
    #ifdef __WXWINCE__
        ::CreateFileForMapping
    #else
        ::CreateFile
    #endif
            (filePath.c_str(), dwDesiredFileAccess, IsReadOnly() ? FILE_SHARE_READ : FILE_SHARE_READ|FILE_SHARE_WRITE,
             0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN|SECURITY_SQOS_PRESENT|SECURITY_IDENTIFICATION, 0);
    if (INVALID_HANDLE_VALUE == m_hFile)
        {
        wxLogWarning(L"Unable to map file (%s): %s",
            (ERROR_SHARING_VIOLATION == ::GetLastError()) ? L"sharing violation" : L"unable to get file handle",
            GetFilePath());
        if (autoBufferOnException && Buffer())
            {
            m_open = true;
            return true;
            }
        else if (ERROR_SHARING_VIOLATION == ::GetLastError() )
            {
            if (autoBufferOnException)
                { wxLogError(L"Unable to map or buffer file (sharing violation): %s", GetFilePath()); }
            Reset();
            throw MemoryMappedFileShareViolationException();
            }
        else
            {
            if (autoBufferOnException)
                { wxLogError(L"Unable to map or buffer file (%s): %s", wxString(wxSysErrorMsg(wxSysErrorCode())), GetFilePath()); }
            Reset();
            throw MemoryMappedFileException();
            }
        }
    //this will fail if the file path was really a drive or printer (don't want to map that!)
    else if (FILE_TYPE_DISK != ::GetFileType(m_hFile))
        {
        wxLogWarning(L"Failed to map a disk or printer: %s", GetFilePath());
        ::CloseHandle(m_hFile);
        Reset();
        throw MemoryMappedInvalidFileType();
        }

    //Get the length of the file
    try
        { m_mapSize = GetFileSize64(m_hFile).GetLo(); }
    catch (MemoryMappedInvalidFileSize)
        {
        m_mapSize = ::SetFilePointer(m_hFile, 0, NULL, FILE_END);
        if (0 == m_mapSize || INVALID_SET_FILE_POINTER == m_mapSize)
            {
            ::CloseHandle(m_hFile);
            if (autoBufferOnException && Buffer())
                {
                m_open = true;
                return true;
                }
            else
                {
                Reset();
                throw MemoryMappedFileEmptyException();
                }
            }
        ::SetFilePointer(m_hFile, 0, NULL, FILE_BEGIN);
        }
    //now, we create a file mapping object for that file
    m_hsection = ::CreateFileMapping(m_hFile, 0, IsReadOnly() ? PAGE_READONLY : PAGE_READWRITE, 0, 0, NULL);
    if (NULL == m_hsection)
        {
        if (m_mapSize > 0)
            { wxLogWarning(L"Unable to create file map (%s): %s", wxString(wxSysErrorMsg(wxSysErrorCode())), GetFilePath()); }
        ::CloseHandle(m_hFile);
        if (wxSysErrorCode() == ERROR_CLOUD_FILE_PROVIDER_NOT_RUNNING ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_METADATA_CORRUPT ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_METADATA_TOO_LARGE ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_PROPERTY_BLOB_TOO_LARGE ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_PROPERTY_BLOB_CHECKSUM_MISMATCH ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_TOO_MANY_PROPERTY_BLOBS ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_PROPERTY_VERSION_NOT_SUPPORTED ||
            wxSysErrorCode() == ERROR_NOT_A_CLOUD_FILE ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_NOT_IN_SYNC ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_ALREADY_CONNECTED ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_NOT_SUPPORTED ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_INVALID_REQUEST ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_READ_ONLY_VOLUME ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_CONNECTED_PROVIDER_ONLY ||
            wxSysErrorCode() == ERROR_CLOUD_FILE_VALIDATION_FAILED)
            {
            Reset();
            throw MemoryMappedFileCloudFileError();
            }
        else if (autoBufferOnException && Buffer())
            {
            m_open = true;
            return true;
            }
        else
            {
            Reset();
            throw MemoryMappedFileException();
            }
        }
    m_data = ::MapViewOfFile(m_hsection, IsReadOnly() ? FILE_MAP_READ : (FILE_MAP_READ|FILE_MAP_WRITE), 0, 0, 0);
    if (nullptr == m_data)
        {
        if (m_mapSize > 0)
            { wxLogWarning(L"Unable to map view of file (%s): %s", wxString(wxSysErrorMsg(wxSysErrorCode())), GetFilePath()); }
        if (autoBufferOnException && Buffer())
            {
            m_open = true;
            return true;
            }
        else
            {
            Reset();
            throw MemoryMappedFileException();
            }
        }
#else
    m_hFile = open(filePath.mb_str(), readOnly ? O_RDONLY : O_RDWR);
    if  (-1 == m_hFile)
        {
        wxLogWarning(L"Unable to map file (unable to get file handle): %s", GetFilePath());
        if (autoBufferOnException && Buffer())
            {
            m_open = true;
            return true;
            }
        else
            {
            Reset();
            throw MemoryMappedFileException();
            }
        }
    //get the size of the file
    m_mapSize = GetFileSize64(m_hFile).GetLo();
    if (static_cast<size_t>(-1) == m_mapSize || 0 == m_mapSize)
        {
        wxLogWarning(L"Unable to map file (empty file): %s", GetFilePath());
        close(m_hFile);
        if (autoBufferOnException && Buffer())
            {
            m_open = true;
            return true;
            }
        else
            {
            Reset();
            throw MemoryMappedFileEmptyException();
            }
        }
    //now get a map of the file
    m_data = mmap(NULL, m_mapSize, readOnly ? PROT_READ : (PROT_READ|PROT_WRITE),
                 (MAP_FILE|MAP_SHARED), m_hFile, 0);
    if (MAP_FAILED == m_data)
        {
        wxLogWarning(L"Unable to map file (general mapping error): %s", GetFilePath());
        close(m_hFile);
        if (autoBufferOnException && Buffer())
            {
            m_open = true;
            return true;
            }
        else
            {
            Reset();
            throw MemoryMappedFileException();
            }
        }
#endif
    m_open = true;
    return true;
    }

///closes the handles and mappings
//-----------------------------------------------
void MemoryMappedFile::UnmapFile()
    {
    if (IsBuffered())
        {
        Reset();
        return;
        }
#ifdef __WXMSW__
    if (m_data)
        {
        ::FlushViewOfFile(m_data, 0);
        ::UnmapViewOfFile(m_data);
        m_data = nullptr;
        }
    if (m_hsection)
        {
        ::CloseHandle(m_hsection);
        m_hsection = NULL;
        }
    if (INVALID_HANDLE_VALUE != m_hFile)
        {
        ::CloseHandle(m_hFile);
        m_hFile = INVALID_HANDLE_VALUE;
        }
#else
    munmap(m_data, static_cast<size_t>(m_mapSize));
    m_data = NULL;
    close(m_hFile);
    m_hFile = -1;
#endif
    Reset();
    }

//-----------------------------------------------
bool MemoryMappedFile::Buffer()
    {
    wxLogMessage(L"Unable to map file, switching to buffering mode: %s", GetFilePath());
    Reset(true);
    wxFile theFile;
    if (!theFile.Open(GetFilePath(), wxFile::read) ) // best to fall back to read only mode if we had to buffer
        {
        wxLogError(L"Unable to open file for buffering: %s", GetFilePath());
        wxMessageBox(wxString::Format(_(L"Unable to open file for buffering:\n%s"), GetFilePath()),
                    _(L"Read Error"), wxOK|wxICON_EXCLAMATION);
        return false;
        }
    try
        { m_bufferedData = new char[theFile.Length()+1]; }
    catch (const std::bad_alloc&)
        {
        wxLogError(L"Not enough memory to open file: %s", GetFilePath());
        wxMessageBox(wxString::Format(_(L"Not enough memory to open file:\n%s"), GetFilePath()),
                    _(L"Read Error"), wxOK|wxICON_EXCLAMATION);
        return false;
        }
    std::memset(m_bufferedData, 0, theFile.Length()+1);
    m_mapSize = theFile.Read(m_bufferedData, theFile.Length());
    m_isBuffered = true;
    return true;
    }

//-----------------------------------------------
void MemoryMappedFile::Reset(const bool preserveFileName /*= false*/)
    {
#ifdef __WXMSW__
    m_hFile = INVALID_HANDLE_VALUE;
    m_hsection = NULL;
#else
    m_hFile = -1;
#endif
    m_data = nullptr;
    wxDELETE(m_bufferedData);
    m_mapSize = 0;
    m_open = false;
    m_isReadOnly = true;
    m_isBuffered = false;
    if (!preserveFileName)
        { m_filePath.clear(); }
    }

//-----------------------------------------------
wxULongLong MemoryMappedFile::GetFileSize64(const MemoryMappedFileHandleType hFile)
    {
#ifdef __WXMSW__
    if (hFile == INVALID_HANDLE_VALUE)
        { throw MemoryMappedInvalidFileSize(); }
    //this will fail if the file path was really a drive or printer
    if (FILE_TYPE_DISK != ::GetFileType(hFile))
        { throw MemoryMappedInvalidFileSize(); }

    LARGE_INTEGER size;
    if (!::GetFileSizeEx(hFile, &size))
        { throw MemoryMappedInvalidFileSize(); }
    else
        { return size.QuadPart; }
#elif defined(__WXOSX__)
    off_t size = lseek(hFile, 0, SEEK_END);
    lseek(hFile, 0, SEEK_SET);//go back to the start of the file
    if (-1 == size)
        { throw MemoryMappedInvalidFileSize(); }
    return wxULongLong(size);
#else
    off64_t size = lseek64(hFile, 0, SEEK_END);
    lseek64(hFile, 0, SEEK_SET);//go back to the start of the file
    if (-1 == size)
        { throw MemoryMappedInvalidFileSize(); }
    return wxULongLong(size);
#endif
    }
