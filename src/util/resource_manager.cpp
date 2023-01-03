///////////////////////////////////////////////////////////////////////////////
// Name:        resource_manager.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "resource_manager.h"
#include "../ui/mainframe.h"
#include "../base/image.h"

//---------------------------------------------------
void ResourceManager::LoadArchive(const wxString& resourceArchivePath)
    {
    // assign if an absolute path
    wxFileName fn(resourceArchivePath);
    if (fn.FileExists())
        {
        if (!fn.IsAbsolute())
            { fn.MakeAbsolute(); }
        m_resourceFile = fn.GetFullPath();
        }
    else
        {
        wxMessageBox(
            wxString::Format(
                _("'%s': resource archive file missing. Please reinstall."), resourceArchivePath),
                _("Error"), wxOK|wxICON_EXCLAMATION);
        m_resourceFile.Clear();
        return;
        }
    try
        {
        if (m_zipFile.MapFile(m_resourceFile, true, true))
            { m_zipCatalog.Init(m_zipFile.GetStream(), m_zipFile.GetMapSize()); }
        }
    catch (...)
        {
        wxMessageBox(_("Cannot open resource collection file."), 
            _("Error"), wxOK|wxICON_EXCLAMATION);
        m_resourceFile.Clear();
        return;
        }
    }

//---------------------------------------------------
wxBitmap ResourceManager::ExtractBitmap(const wxString& bmpPath,
                                          const wxBitmapType bitmapType) const
    { return m_zipCatalog.ReadBitmap(bmpPath, bitmapType); }

//---------------------------------------------------
wxBitmap ResourceManager::GetBitmap(const wxString& filePath, const wxBitmapType bitmapType)
    {
    const auto imagePos = m_imageMap.find(filePath);
    if (imagePos == m_imageMap.cend())
        {
        // load bitmap from disk if a local file
        if (wxFile::Exists(filePath))
            {
            wxImage img = Wisteria::GraphItems::Image::LoadFile(filePath);
            if (!img.IsOk())
                { return wxNullBitmap; }
            wxLogVerbose(
                wxString::Format(L"%s extracted from file. Width=%d, Height=%d",
                                 filePath, img.GetWidth(), img.GetHeight()));
            return m_imageMap[filePath] = wxBitmap(img);
            }
        // ...otherwise, load from the resource zip file
        else
            {
            wxBitmap bmp = ExtractBitmap(filePath, bitmapType);
            wxASSERT_LEVEL_2_MSG(bmp.IsOk(),
                wxString::Format(L"%s: failed to load image from resoures.",
                                 filePath));
            wxLogVerbose(wxString::Format(L"%s extracted from resource file. Width=%d, Height=%d",
                                          filePath, bmp.GetWidth(), bmp.GetHeight()));
            return m_imageMap[filePath] = bmp;
            }
        }
    else
        { return imagePos->second; }
    }

//-------------------------------------------------------
wxBitmapBundle ResourceManager::GetSVG(const wxString& path)
    {
    const auto imagePos = m_bmpBundleMap.find(path);
    if (imagePos == m_bmpBundleMap.cend())
        {
        // load bitmap from disk if a local file
        if (wxFile::Exists(path))
            {
            wxASSERT_MSG(wxBitmapBundle::FromSVGFile(path, wxSize(16, 16)).IsOk(),
                wxString::Format(L"Failed to load '%s' SVG icon", path));

            std::vector<wxBitmap> bmps =
                {
                wxBitmapBundle::FromSVGFile(path, wxSize(16, 16)).GetBitmap(wxSize(16, 16)),
                wxBitmapBundle::FromSVGFile(path, wxSize(32, 32)).GetBitmap(wxSize(32, 32)),
                wxBitmapBundle::FromSVGFile(path, wxSize(64, 64)).GetBitmap(wxSize(64, 64)),
                wxBitmapBundle::FromSVGFile(path, wxSize(128, 128)).GetBitmap(wxSize(128, 128))
                };

            const auto[node, inserted] =
                m_bmpBundleMap.insert(std::make_pair(path, wxBitmapBundle::FromBitmaps(bmps)));

            return node->second;
            }
        else
            {
            wxASSERT_MSG(m_zipCatalog.ReadSVG(path, wxSize(16, 16)).IsOk(),
                wxString::Format(L"Failed to load '%s' SVG icon", path));

            std::vector<wxBitmap> bmps =
                {
                m_zipCatalog.ReadSVG(path, wxSize(16, 16)),
                m_zipCatalog.ReadSVG(path, wxSize(32, 32)),
                m_zipCatalog.ReadSVG(path, wxSize(64, 64)),
                m_zipCatalog.ReadSVG(path, wxSize(128, 128))
                };

            const auto[node, inserted] =
                m_bmpBundleMap.insert(std::make_pair(path, wxBitmapBundle::FromBitmaps(bmps)));

            return node->second;
            }
        }
    else
        { return imagePos->second; }
    }
