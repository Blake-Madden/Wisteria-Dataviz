///////////////////////////////////////////////////////////////////////////////
// Name:        zipcatalog.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "zipcatalog.h"

using namespace Wisteria;

//------------------------------------------------
ZipCatalog::ZipCatalog(const void* data, size_t len) :
    m_inzip(nullptr), m_readErrorShown(false)
    { Init(data,len); }

//------------------------------------------------
ZipCatalog::ZipCatalog(const wxString& zipFilePath) :
    m_inzip(nullptr), m_readErrorShown(false)
    {
    try
        {
        if (wxFileName::FileExists(zipFilePath) && m_mapfile.MapFile(zipFilePath, true, true))
            { Init(static_cast<char*>(m_mapfile.GetStream()), m_mapfile.GetMapSize()); }
        else
            { wxLogError(L"Error reading ZIP file: %s", zipFilePath); }
        }
    catch (...)
        { wxLogError(L"Error reading ZIP file: %s", zipFilePath); }
    }

//------------------------------------------------
void ZipCatalog::Init(const void* data, size_t len)
    {
    // reset data in case a new archive is being used now
    for (auto& entry : m_catalog)
        { wxDELETE(entry.second); }
    m_catalog.clear();
    ClearMessages();
    wxDELETE(m_inzip);

    m_inzip = new wxZipInputStream(new wxMemoryInputStream(data,len));
    wxZipEntry* entry{ nullptr };
    // load the zip catalog (just files, no folders)
    while ((entry = m_inzip->GetNextEntry()) != nullptr)
        {
        if (!entry->IsDir())
            { m_catalog[entry->GetInternalName()] = entry; }
        else
            { wxDELETE(entry); }
        }
    }

//------------------------------------------------
ZipCatalog::~ZipCatalog()
    {
    for (auto& entry : m_catalog)
        { wxDELETE(entry.second); }
    wxDELETE(m_inzip); // will implicitly delete the memory stream that it is wrapping
    }

//------------------------------------------------
bool ZipCatalog::ReadFile(const wxString& path, wxOutputStream& memstream) const
    {
    if (!m_inzip)
        { return false; }
    wxZipEntry* entry = Find(path);
    if (!entry)
        {
        wxLogWarning(L"%s: file not found in zip file.", path);
        return false;
        }
    else if (entry->GetCompressedSize() == 0)
        { return false; }

    if (m_inzip->OpenEntry(*entry))
        {
        const bool retVal = Read(m_inzip, memstream, entry->GetCompressedSize());
        m_inzip->CloseEntry();
        return retVal;
        }

    return false;
    }

//------------------------------------------------
wxString ZipCatalog::ReadTextFile(const wxString& path) const
    {
    // unzip the file into a temp file...
    const wxString tempFilePath = wxFileName::CreateTempFileName(wxStandardPaths::Get().GetTempDir() +
        wxFileName::GetPathSeparator() + L"RS");
    wxFileOutputStream tempFile(tempFilePath);
    if (!ReadFile(path, tempFile))
        {
        tempFile.Close();
        wxRemoveFile(tempFilePath);
        return wxEmptyString;
        }
    tempFile.Close();

    // ...then memory map it and convert it to Unicode and return that.
    MemoryMappedFile mappedTempFile;
    try
        { mappedTempFile.MapFile(tempFilePath, true, true); }
    catch (...)
        {
        wxMessageBox(wxString::Format(
            _("Error reading extracted file from temp folder: %s"), path), 
            _("Read Error"), wxOK|wxICON_EXCLAMATION);
        return wxEmptyString;
        }
    if (mappedTempFile.IsOk())
        {
        const wxString decodedText = Wisteria::TextStream::CharStreamToUnicode(
            static_cast<const char*>(mappedTempFile.GetStream()), mappedTempFile.GetMapSize());
        mappedTempFile.UnmapFile();
        wxRemoveFile(tempFilePath);
        return decodedText;
        }
    else
        {
        wxMessageBox(wxString::Format(
            _("Error reading extracted file from temp folder: %s"), path), 
            _("Read Error"), wxOK|wxICON_EXCLAMATION);
        wxRemoveFile(tempFilePath);
        return wxEmptyString;
        }
    }

//------------------------------------------------
wxString ZipCatalog::ExtractTextFileToTempFile(const wxString& path) const
    {
    // unzip the file into a temp file...
    const wxString charStreamTempFilePath = wxFileName::CreateTempFileName(
        wxStandardPaths::Get().GetTempDir() + wxFileName::GetPathSeparator() + L"RS");
    wxFileOutputStream charStreamTempFile(charStreamTempFilePath);
    if (!ReadFile(path, charStreamTempFile))
        {
        charStreamTempFile.Close();
        wxRemoveFile(charStreamTempFilePath);
        return wxEmptyString;
        }
    charStreamTempFile.Close();

    // map the char* data and convert it to Unicode (into another temp file)
    MemoryMappedFile mappedTempFile;
    try
        { mappedTempFile.MapFile(charStreamTempFilePath, true, true); }
    catch (...)
        {
        wxMessageBox(wxString::Format(
            _("Error reading extracted file from temp folder: %s"), path), 
            _("Read Error"), wxOK|wxICON_EXCLAMATION);
        return wxEmptyString;
        }
    if (mappedTempFile.IsOk())
        {
        // temp file for the converted text into
        const wxString UnicodeTempFilePath = wxFileName::CreateTempFileName(
            wxStandardPaths::Get().GetTempDir() + wxFileName::GetPathSeparator() + L"RS");
        // Dump the char* buffer a few times into it to make it big enough to
        // hold the converted data.
            {
            wxFileOutputStream UnicodeTempFile(UnicodeTempFilePath);
            UnicodeTempFile.Write(
                static_cast<const char*>(mappedTempFile.GetStream()),
                                         mappedTempFile.GetMapSize()).
                Write(static_cast<const char*>(mappedTempFile.GetStream()),
                                               mappedTempFile.GetMapSize()).
                Write(static_cast<const wchar_t*>(mappedTempFile.GetStream()),
                                                  mappedTempFile.GetMapSize()).
                Write(L"\0\0", sizeof(wchar_t)); // space for a null terminator
            }
        // convert the text directly into the unicode temp file
            {
            MemoryMappedFile mappedUnicodeFile;
            try
                { mappedUnicodeFile.MapFile(UnicodeTempFilePath, false, true); }
            catch (...)
                {
                wxMessageBox(wxString::Format(
                    _("Error reading extracted file from temp folder: %s"), path), 
                    _("Read Error"), wxOK|wxICON_EXCLAMATION);
                return wxEmptyString;
                }
            if (mappedUnicodeFile.IsOk())
                {
                Wisteria::TextStream::CharStreamToUnicode(
                    static_cast<wchar_t*>(mappedUnicodeFile.GetStream()),
                                          (mappedUnicodeFile.GetMapSize()/sizeof(wchar_t)),
                    static_cast<const char*>(mappedTempFile.GetStream()),
                                             mappedTempFile.GetMapSize());
                }
            else
                {
                wxLogError(L"Error writing extracted file to temp folder: %s", path);
                wxMessageBox(wxString::Format(
                    _("Error writing extracted file to temp folder: %s"), path), 
                    _("Read Error"), wxOK|wxICON_EXCLAMATION);
                mappedUnicodeFile.UnmapFile();
                wxRemoveFile(UnicodeTempFilePath);
                return wxEmptyString;
                }
            }
        mappedTempFile.UnmapFile();
        wxRemoveFile(charStreamTempFilePath);

        return UnicodeTempFilePath;
        }
    else
        {
        wxMessageBox(wxString::Format(
            _("Error reading extracted file from temp folder: %s"), path), 
            _("Read Error"), wxOK|wxICON_EXCLAMATION);
        wxRemoveFile(charStreamTempFilePath);
        return wxEmptyString;
        }
    }

//------------------------------------------------
wxBitmap ZipCatalog::ReadBitmap(const wxString& path, const wxBitmapType bitmapType) const
    {
    wxMemoryOutputStream memstream;
    if (!ReadFile(path, memstream))
        { return wxNullBitmap; }
    // convert it from the stream
    wxMemoryInputStream stream(memstream.GetOutputStreamBuffer()->GetBufferStart(),
                               memstream.GetLength());
    wxImage img;
    if (!img.LoadFile(stream,bitmapType))
        { return wxNullBitmap; }

    return wxBitmap(img);
    }

//------------------------------------------------
bool ZipCatalog::Read(wxInputStream* stream_in, wxOutputStream& stream_out,
                      const size_t bufferSize) const
    {
    if (bufferSize == 0)
        {
        stream_out.Reset();
        return false;
        }
    auto buf = std::make_unique<char[]>(bufferSize);

    for (;;)
        {
        const size_t bytes_read = stream_in->Read(buf.get(), bufferSize).LastRead();
        if ( !bytes_read )
            break;

        if ( stream_out.Write(buf.get(), bytes_read).LastWrite() != bytes_read )
            break;
        }
    if (stream_out.GetLength() == 0 && !m_readErrorShown)
        {
        m_messages.emplace_back(
            _("Unable to read file from archive. Archive may be corrupt or password protected."),
            wxICON_EXCLAMATION);
        m_readErrorShown = true;
        return false;
        }
    return true;
    }

//------------------------------------------------
void ZipCatalog::WriteText(wxZipOutputStream& zip, const wxString& fileName, const wxString& text)
    {
    /* Use buffered output stream, NOT text output stream. Text output buffer
       messes around with the newlines in the text, whereas buffer streams preserve the text.*/
    wxBufferedOutputStream txt(zip);
    zip.PutNextEntry(fileName);
    wxCharBuffer fileTextConvertedBuffer = text.mb_str(wxConvUTF8);
    zip.Write(lily_of_the_valley::unicode_extract_text::get_bom_utf8(),
              std::strlen(lily_of_the_valley::unicode_extract_text::get_bom_utf8()));
    wxASSERT_MSG(fileTextConvertedBuffer.length() == std::strlen(fileTextConvertedBuffer.data()),
        L"Invalid buffer size when writing from ZipCatalog!");
    zip.Write(fileTextConvertedBuffer.data(), fileTextConvertedBuffer.length());
    txt.Close();
    }

//------------------------------------------------
wxArrayString ZipCatalog::FindFilesInFolder(const wxString& path) const
    {
    wxArrayString files;
    wxString formattedPath = path;
    if (path.length() &&
        path[path.length()-1] != L'/' && path[path.length()-1] != L'\\')
        { formattedPath = path + L'/'; }
    for (const auto& entry : m_catalog)
        {
        if (entry.first.StartsWith(wxZipEntry::GetInternalName(formattedPath)) )
            { files.Add(wxZipEntry::GetInternalName(entry.first)); }
        }
    return files;
    }

//------------------------------------------------
wxArrayString ZipCatalog::GetPaths() const
    {
    wxArrayString files;
    files.Alloc(m_catalog.size());
    for (const auto& entry : m_catalog)
        { files.Add(wxZipEntry::GetInternalName(entry.first)); }
    return files;
    }
