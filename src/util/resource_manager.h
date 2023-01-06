/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __RESOURCE_MANAGER_H__
#define __RESOURCE_MANAGER_H__

#include <map>
#include <wx/wx.h>
#include <wx/fs_zip.h>
#include <wx/filesys.h>
#include <wx/wfstream.h>
#include <wx/stdpaths.h>
#include <wx/mstream.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/manager.h>
#include <wx/listctrl.h>
#include <wx/ribbon/bar.h>
#include <wx/ribbon/buttonbar.h>
#include "memorymappedfile.h"
#include "donttranslate.h"
#include "zipcatalog.h"
#include "../math/mathematics.h"

/// @brief Class to manage loading images, icons, and XRC files from a ZIP file.
/// @details Also supports local files as well.\n
///     Images (and @c wxBitmapBundles) loaded through this class will be cached,
///     so that the next time they are requested they will not need to be reloaded.
class ResourceManager
    {
public:
    /// @private
    ResourceManager() = default;
    /** @brief Constructor.
        @param resourceArchivePath The path to the ZIP file containing the resources
            to load.*/
    explicit ResourceManager(const wxString& resourceArchivePath)
        { LoadArchive(resourceArchivePath); }
    /// @private
    ResourceManager(const ResourceManager&) = delete;
    /// @private
    ResourceManager(ResourceManager&&) = delete;
    /// @private
    ResourceManager& operator=(const ResourceManager&) = delete;
    /// @private
    ResourceManager& operator=(ResourceManager&&) = delete;
    /** @brief Loads the archive file (must be in ZIP format) to extract resources from.
        @param resourceArchivePath The path to the ZIP file containing the resources
            to use for the application.
        @param Only one archive can be loaded at a time; calling this will unload
            any previoulys loaded ZIP file.*/
    void LoadArchive(const wxString& resourceArchivePath);

    /** @returns The @c wxWidgets file-system path to a file in the loaded archive.
        @param subFile The file to look for in the archive.*/
    [[nodiscard]]
    wxString GetResourceFilePath(const wxString& subFile = wxEmptyString) const
        {
        if (subFile.empty())
            { return m_resourceFile; }
        else
            { return m_resourceFile + _DT(L"#zip:") + subFile; }
        }

    /// @returns A bitmap from the provided path.
    /// @param filePath The path to the image.\n
    ///     Can be relative to the ZIP file loaded by this class, or a local file.
    /// @param bitmapType The image's type.
    [[nodiscard]]
    wxBitmap GetBitmap(const wxString& filePath, const wxBitmapType bitmapType);

    /// @returns A bitmap bundle from the provided path.
    /// @param path The path to the image.\n
    ///     Can be relative to the ZIP file loaded by this class, or a local file.
    /// @note The returned bundle will ccontain 16x16, 32x32, 64x64, and 128x128
    ///     copies of the image.
    [[nodiscard]]
    wxBitmapBundle GetSVG(const wxString& path);

    /** @returns A list of files in a given folder
            (relative to its location in the loaded archive's folder structure).
        @param path The name of the folder (in the attached archive file) to iterate.*/
    [[nodiscard]]
    wxArrayString GetFilesInFolder(const wxString& path) const
        { return m_zipCatalog.GetFilesInFolder(path); }

    /// @brief Creates an icon filled with the specified color.
    /// @param color The color of the icon.
    /// @returns The color-filled icon.
    [[nodiscard]]
    static wxBitmapBundle CreateColorIcon(const wxColour& color);
private:
    wxBitmap ExtractBitmap(const wxString& bmpPath, const wxBitmapType bitmapType) const;
    wxString m_resourceFile;
    std::map<wxString, wxBitmap> m_imageMap;
    std::map<wxString, wxBitmapBundle> m_bmpBundleMap;
    Wisteria::ZipCatalog m_zipCatalog;
    MemoryMappedFile m_zipFile;
    };

/** @}*/

#endif //__RESOURCE_MANAGER_H__
