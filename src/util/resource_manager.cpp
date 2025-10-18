///////////////////////////////////////////////////////////////////////////////
// Name:        resource_manager.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "resource_manager.h"
#include "../base/image.h"
#include "../ui/mainframe.h"
#include <wx/fs_zip.h>
#include <wx/propgrid/manager.h>
#include <wx/propgrid/propgrid.h>
#include <wx/wfstream.h>

//---------------------------------------------------
void ResourceManager::LoadArchive(const wxString& resourceArchivePath)
    {
    // assign if an absolute path
    if (wxFileName fn(resourceArchivePath); fn.FileExists())
        {
        if (!fn.IsAbsolute())
            {
            fn.MakeAbsolute();
            }
        m_resourceFile = fn.GetFullPath();
        }
    else
        {
        wxMessageBox(wxString::Format(_(L"'%s': resource archive file missing. Please reinstall."),
                                      resourceArchivePath),
                     _(L"Error"), wxOK | wxICON_EXCLAMATION);
        m_resourceFile.Clear();
        return;
        }
    try
        {
        if (m_zipFile.MapFile(m_resourceFile, true, true))
            {
            m_zipCatalog.Init(m_zipFile.GetStream(), m_zipFile.GetMapSize());
            }
        }
    catch (...)
        {
        wxMessageBox(_(L"Cannot open resource collection file."), _(L"Error"),
                     wxOK | wxICON_EXCLAMATION);
        m_resourceFile.Clear();
        }
    }

//---------------------------------------------------
wxBitmap ResourceManager::ExtractBitmap(const wxString& bmpPath,
                                        const wxBitmapType bitmapType) const
    {
    return m_zipCatalog.ReadBitmap(bmpPath, bitmapType);
    }

//---------------------------------------------------
wxBitmap ResourceManager::GetBitmap(const wxString& filePath, const wxBitmapType bitmapType)
    {
    const auto imagePos = m_imageMap.find(filePath);
    if (imagePos == m_imageMap.cend())
        {
        // load bitmap from disk if a local file
        if (wxFile::Exists(filePath))
            {
            const wxImage img = Wisteria::GraphItems::Image::LoadFile(filePath);
            if (!img.IsOk())
                {
                return wxNullBitmap;
                }
            wxLogDebug(L"%s extracted from file. Width=%d, Height=%d", filePath, img.GetWidth(),
                       img.GetHeight());
            return m_imageMap[filePath] = wxBitmap(img);
            }
        // ...otherwise, load from the resource zip file

        const wxBitmap bmp = ExtractBitmap(filePath, bitmapType);
        wxASSERT_MSG(bmp.IsOk(), "Failed to load image from resources!");
        wxLogDebug(L"%s extracted from resource file. Width=%d, Height=%d", filePath,
                   bmp.GetWidth(), bmp.GetHeight());
        return m_imageMap[filePath] = bmp;
        }

    return imagePos->second;
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
            wxASSERT_MSG(wxBitmapBundle::FromSVGFile(path, wxSize{ 16, 16 }).IsOk(),
                         L"Failed to load SVG icon!");

            const wxVector<wxBitmap> bmps{
                wxBitmapBundle::FromSVGFile(path, wxSize{ 16, 16 }).GetBitmap(wxSize{ 16, 16 }),
                wxBitmapBundle::FromSVGFile(path, wxSize{ 32, 32 }).GetBitmap(wxSize{ 32, 32 }),
                wxBitmapBundle::FromSVGFile(path, wxSize{ 64, 64 }).GetBitmap(wxSize{ 64, 64 }),
                wxBitmapBundle::FromSVGFile(path, wxSize{ 128, 128 }).GetBitmap(wxSize{ 128, 128 }),
                wxBitmapBundle::FromSVGFile(path, wxSize{ 256, 256 }).GetBitmap(wxSize{ 256, 256 })
            };

            const auto [node, inserted] =
                m_bmpBundleMap.insert(std::make_pair(path, wxBitmapBundle::FromBitmaps(bmps)));

            return node->second;
            }

        const wxVector<wxBitmap> bmps{ m_zipCatalog.ReadSVG(path, wxSize{ 16, 16 }),
                                       m_zipCatalog.ReadSVG(path, wxSize{ 32, 32 }),
                                       m_zipCatalog.ReadSVG(path, wxSize{ 64, 64 }),
                                       m_zipCatalog.ReadSVG(path, wxSize{ 128, 128 }),
                                       m_zipCatalog.ReadSVG(path, wxSize{ 256, 256 }) };

        const auto [node, inserted] =
            m_bmpBundleMap.insert(std::make_pair(path, wxBitmapBundle::FromBitmaps(bmps)));

        return node->second;
        }

    return imagePos->second;
    }

//-------------------------------------------------------
wxBitmapBundle ResourceManager::CreateColorIcon(const wxColour& color)
    {
    wxASSERT(color.IsOk());
    if (!color.IsOk())
        {
        return wxNullBitmap;
        }

    wxVector<wxBitmap> bmps{ wxBitmap{ 16, 16 }, wxBitmap{ 32, 32 }, wxBitmap{ 64, 64 },
                             wxBitmap{ 128, 128 }, wxBitmap{ 256, 256 } };

    const auto fillIcon = [&color](wxBitmap& bmp)
    {
        wxMemoryDC memDC{ bmp };
        memDC.SetBrush(wxBrush{ color });
        memDC.SetPen(wxColour{ 0, 0, 0 });
        memDC.Clear();
        memDC.DrawRectangle(0, 0, bmp.GetWidth(), bmp.GetHeight());
        memDC.SelectObject(wxNullBitmap);
    };

    std::ranges::for_each(bmps, fillIcon);

    return wxBitmapBundle::FromBitmaps(bmps);
    }
