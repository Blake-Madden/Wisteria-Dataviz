///////////////////////////////////////////////////////////////////////////////
// Name:        zipcatalog.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "zipcatalog.h"
#include "../import/unicode_extract_text.h"
#include "wx/bmpbndl.h"
#include <wx/image.h>
#include <wx/stdpaths.h>
#include <wx/wfstream.h>

using namespace Wisteria;

//------------------------------------------------
ZipCatalog::ZipCatalog(const void* data, size_t len) { Init(data, len); }

//------------------------------------------------
ZipCatalog::ZipCatalog(const wxString& zipFilePath)
    {
    try
        {
        if (wxFileName::FileExists(zipFilePath) && m_mapfile.MapFile(zipFilePath, true, true))
            {
            Init(static_cast<char*>(m_mapfile.GetStream()), m_mapfile.GetMapSize());
            }
        else
            {
            wxLogError(L"Error reading ZIP file: %s", zipFilePath);
            }
        }
    catch (...)
        {
        wxLogError(L"Error reading ZIP file: %s", zipFilePath);
        }
    }

//------------------------------------------------
void ZipCatalog::Init(const void* data, size_t len)
    {
    // reset data in case a new archive is being used now
    for (auto& entry : m_catalog)
        {
        wxDELETE(entry.second);
        }
    m_catalog.clear();
    ClearMessages();
    wxDELETE(m_inzip);

    m_inzip = new wxZipInputStream(new wxMemoryInputStream(data, len));
    wxZipEntry* entry{ nullptr };
    // load the zip catalog (just files, no folders)
    while ((entry = m_inzip->GetNextEntry()) != nullptr)
        {
        if (!entry->IsDir())
            {
            m_catalog[entry->GetInternalName()] = entry;
            }
        else
            {
            wxDELETE(entry);
            }
        }
    }

//------------------------------------------------
ZipCatalog::~ZipCatalog()
    {
    for (auto& entry : m_catalog)
        {
        wxDELETE(entry.second);
        }
    wxDELETE(m_inzip); // will implicitly delete the memory stream that it is wrapping
    }

//------------------------------------------------
bool ZipCatalog::ReadFile(const wxString& path, wxOutputStream& memstream) const
    {
    if (!m_inzip)
        {
        return false;
        }
    wxZipEntry* entry = Find(path);
    if (!entry)
        {
        wxLogWarning(L"%s: file not found in zip file.", path);
        return false;
        }
    else if (entry->GetCompressedSize() == 0)
        {
        return false;
        }

    if (m_inzip->OpenEntry(*entry))
        {
        const bool retVal = Read(m_inzip, memstream, entry->GetCompressedSize());
        m_inzip->CloseEntry();
        return retVal;
        }

    return false;
    }

//------------------------------------------------
std::wstring ZipCatalog::ReadTextFile(const wxString& path) const
    {
    wxMemoryOutputStream memstream;
    if (!ReadFile(path, memstream))
        {
        return {};
        }

    const wxStreamBuffer* theBuffer = memstream.GetOutputStreamBuffer();
    assert(theBuffer && L"Invalid buffer in call to ZipCatalog::ReadTextFile()!");
    // empty file
    if (theBuffer == nullptr || theBuffer->GetBufferStart() == nullptr ||
        theBuffer->GetBufferSize() == 0 || memstream.GetLength() == 0)
        {
        return {};
        }

    return Wisteria::TextStream::CharStreamToUnicode(
        static_cast<const char*>(theBuffer->GetBufferStart()), memstream.GetLength());
    }

//------------------------------------------------
wxBitmap ZipCatalog::ReadSVG(const wxString& path, const wxSize size) const
    {
    wxMemoryOutputStream memstream;
    if (!ReadFile(path, memstream))
        {
        return {};
        }
    // convert it from the stream
    const auto bmp = wxBitmapBundle::FromSVG(
        static_cast<const wxByte*>(memstream.GetOutputStreamBuffer()->GetBufferStart()),
        memstream.GetLength(), size);
    return bmp.GetBitmap(bmp.GetDefaultSize());
    }

//------------------------------------------------
wxBitmap ZipCatalog::ReadBitmap(const wxString& path, const wxBitmapType bitmapType) const
    {
    wxMemoryOutputStream memstream;
    if (!ReadFile(path, memstream))
        {
        return wxNullBitmap;
        }
    // convert it from the stream
    wxMemoryInputStream stream(memstream.GetOutputStreamBuffer()->GetBufferStart(),
                               memstream.GetLength());
    wxImage img;
    if (!img.LoadFile(stream, bitmapType))
        {
        return wxNullBitmap;
        }

    return { img };
    }

//------------------------------------------------
bool ZipCatalog::Read(wxInputStream* stream_in, wxOutputStream& stream_out,
                      const size_t bufferSize) const
    {
    if (stream_in == nullptr || bufferSize == 0)
        {
        // Reset only clears error flags; it doesn't truncate any output target.
        // If you intended to clear the destination, that must be done by the caller.
        stream_out.Reset();
        return false;
        }

    m_readBuffer.resize(bufferSize);
    std::ranges::fill(m_readBuffer, 0);

    size_t totalWritten{ 0 };

    while (stream_in->IsOk() && !stream_in->Eof() && stream_out.IsOk())
        {
        stream_in->Read(m_readBuffer.data(), bufferSize);
        const size_t bytesRead = stream_in->LastRead();
        if (bytesRead == 0)
            {
            break;
            }

        const size_t bytesWrote = stream_out.Write(m_readBuffer.data(), bytesRead).LastWrite();

        totalWritten += bytesWrote;

        if (bytesWrote != bytesRead)
            {
            // short write: stop; let success be judged by totalWritten.
            break;
            }
        }

    if (totalWritten == 0 && !m_readErrorShown)
        {
        m_messages.emplace_back(
            _(L"Unable to read file from archive. Archive may be corrupt or password protected."),
            wxICON_EXCLAMATION);
        m_readErrorShown = true;
        return false;
        }

    // true if we managed to write anything and the sink looks healthy.
    return totalWritten > 0 && stream_out.IsOk();
    }

//------------------------------------------------
void ZipCatalog::WriteText(wxZipOutputStream& zip, const wxString& fileName, const wxString& text)
    {
    // convert first so we don't start an entry we can't finish.
    const wxScopedCharBuffer utf8 = text.ToUTF8();
    if (!utf8)
        {
        wxASSERT_MSG(false, L"UTF-8 conversion in zip file failed");
        return;
        }

    if (!zip.PutNextEntry(fileName))
        {
        wxASSERT_MSG(false, L"PutNextEntry() failed");
        return;
        }

    if (!zip.WriteAll(utf8.data(), utf8.length()) || !zip.IsOk())
        {
        wxASSERT_MSG(false, L"Write to zip file failed");
        }

    zip.CloseEntry();
    }

//------------------------------------------------
wxArrayString ZipCatalog::GetFilesInFolder(const wxString& path) const
    {
    wxArrayString files;
    files.Alloc(m_catalog.size());

    wxString formattedPath = path;
    if (!formattedPath.empty() && formattedPath[formattedPath.length() - 1] != L'/' &&
        formattedPath[formattedPath.length() - 1] != L'\\')
        {
        formattedPath += L'/';
        }

    const wxString prefix = wxZipEntry::GetInternalName(formattedPath);

    for (const auto& entry : m_catalog)
        {
        const wxString name = wxZipEntry::GetInternalName(entry.first);
        if (name.starts_with(prefix) && !name.ends_with(L"/"))
            {
            files.Add(name);
            }
        }

    return files;
    }

//------------------------------------------------
wxArrayString ZipCatalog::GetPaths() const
    {
    wxArrayString files;
    files.Alloc(m_catalog.size());
    for (const auto& entry : m_catalog)
        {
        files.Add(wxZipEntry::GetInternalName(entry.first));
        }
    return files;
    }
