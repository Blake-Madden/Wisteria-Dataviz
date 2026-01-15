/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <initializer_list>
#include <wx/arrstr.h>
#include <wx/filename.h>
#include <wx/string.h>
// defines NOMINMAX, include it before windows.h
#include <wx/wx.h>
#ifdef __WXMSW__
    #include <shlobj.h>
    #include <windows.h>
#endif
#include "../data/dataset.h"
#include "../util/donttranslate.h"

/// @brief The filepaths that a string may be resolved to.
enum class FilePathType
    {
    HTTP,            /*!< HTTP URL.*/
    HTTPS,           /*!< HTTPS (secure) URL.*/
    FTP,             /*!< FTP URL.*/
    FTPS,            /*!< FTPS (secure) URL.*/
    Gopher,          /*!< Gopher URL.*/
    LocalOrNetwork,  /*!< Path that is either on the local system or on a network.*/
    InvalidFileType, /*!< Not a legitimate file path.*/
    ArchivedFile,    /*!< A file inside an archive file.
                        May or may not be an URL or local file--caller is responsible for
                        determining that and opening it accordingly.
                        @note The path syntax is `path/file.zip#subfile`.*/
    ExcelCell        /*!< A cell address inside an Excel 2007 file.
                        May or may not be an URL or local file--caller is responsible for
                        determining that and opening it accordingly.
                        @note The path syntax is `path/file.xlsx#sheet_name#cell`.*/
    };

/** @brief Class to determine which sort of filepath a string may resemble.
    @details This is useful for determining if a string is a file to a file or URL,
        and determining specifically which sort of path it is.*/
class FilePathResolverBase
    {
  public:
    /// @private
    FilePathResolverBase() = default;

    /** @brief Resolves a string to see if it is a file path.
        @param path The string to resolve to a file path.
        @note Set @c attemptToConnect to false if performance is a concern.*/
    explicit FilePathResolverBase(const wxString& path) { ResolvePath(path); }

    /** @brief Resolves a string to see if it is a file path.
        @param path The string to resolve to a file path.
        @param pathsToSearch A list of local paths to look in if @c path is a relative local path.
        @returns The resolved path.*/
    wxString
    ResolvePath(const wxString& path,
                std::initializer_list<wxString> pathsToSearch = std::initializer_list<wxString>{});

    /** @returns The (possibly) corrected path that the supplied path was resolved to.
            This includes correcting slashes, encoding spaces, and stripping
            file protocol prefixes.*/
    [[nodiscard]]
    wxString GetResolvedPath() const
        {
        return m_path;
        }

    /// @returns @c true if filepath is on the local system or a network
    ///     (e.g., network drive or UNC path).
    [[nodiscard]]
    bool IsLocalOrNetworkFile() const noexcept
        {
        return m_fileType == FilePathType::LocalOrNetwork;
        }

    /** @returns @c true if filepath is an internet URL.
        @note This encompasses HTTP, HTTPS, FTP, and Gopher paths.
            (And "www" paths if the prefix is missing).*/
    [[nodiscard]]
    bool IsWebFile() const noexcept
        {
        return (m_fileType == FilePathType::HTTP || m_fileType == FilePathType::HTTPS ||
                m_fileType == FilePathType::FTP || m_fileType == FilePathType::Gopher);
        }

    /** @returns @c true if an HTTP path.
        @note Prefer IsWebFile() if simply needing to verify that a path is an URL.*/
    [[nodiscard]]
    bool IsHTTPFile() const noexcept
        {
        return m_fileType == FilePathType::HTTP;
        }

    /** @returns @c true if an HTTPS path.
        @note Prefer IsWebFile() if simply needing to verify that a path is an URL.*/
    [[nodiscard]]
    bool IsHTTPSFile() const noexcept
        {
        return m_fileType == FilePathType::HTTPS;
        }

    /** @returns @c true if an FTP path.
        @note Prefer IsWebFile() if simply needing to verify that a path is an URL.*/
    [[nodiscard]]
    bool IsFTPFile() const noexcept
        {
        return m_fileType == FilePathType::FTP;
        }

    /** @returns @c true if a Gopher path.
        @note Prefer IsWebFile() if simply needing to verify that a path is an URL.*/
    [[nodiscard]]
    bool IsGopherFile() const noexcept
        {
        return m_fileType == FilePathType::Gopher;
        }

    /** @returns @c true if the text supplied didn't appear to be any sort of filepath or URL.*/
    [[nodiscard]]
    bool IsInvalidFile() const noexcept
        {
        return m_fileType == FilePathType::InvalidFileType;
        }

    /** @returns @c true if path is a file inside an archive file.
        @note The file may actually be local (or on a network),
        but because we use special syntax for paths to archive subfiles
        then we have to consider its path type as something special.*/
    [[nodiscard]]
    bool IsArchivedFile() const noexcept
        {
        return m_fileType == FilePathType::ArchivedFile;
        }

    /** @returns @c true if path is a cell inside an Excel 2007 file.
        @note The file may actually be local (or on a network),
            but because we use special syntax for paths to Excel cells
            then we have to consider its path type as something special.*/
    [[nodiscard]]
    bool IsExcelCell() const noexcept
        {
        return m_fileType == FilePathType::ExcelCell;
        }

    /// @returns @c true if filepath has a supported spreadsheet extension.
    /// @param fn The filepath to review.
    [[nodiscard]]
    static bool IsSpreadsheet(const wxFileName& fn)
        {
        return (fn.GetExt().CmpNoCase(L"xlsx") == 0);
        }

    /// @returns @c true if filepath has a supported archive extension.
    /// @param fn The filepath to review.
    [[nodiscard]]
    static bool IsArchive(const wxFileName& fn)
        {
        return (fn.GetExt().CmpNoCase(_DT(L"zip")) == 0);
        }

    /// @returns The specific type of filepath detected.
    [[nodiscard]]
    FilePathType GetFileType() const noexcept
        {
        return m_fileType;
        }

  protected:
    /// @private
    [[nodiscard]]
    static bool HasWindowsPrefix(const wxString& str);
    /// @private
    [[nodiscard]]
    static bool HasUnixPrefix(const wxString& str);
    /// @private
    [[nodiscard]]
    static bool HasNetworkPrefix(const wxString& str);

    /// @private
    [[nodiscard]]
    static bool HasLocalOrNetworkPrefix(const wxString& str)
        {
        return (HasWindowsPrefix(str) || HasUnixPrefix(str) || HasNetworkPrefix(str));
        }

    /// @private
    wxString m_path;
    /// @private
    FilePathType m_fileType{ FilePathType::InvalidFileType };
    };

/** @brief Renames a file, attempting to shortening the destination name.
    @details If the destination file name is more than 255 and the original isn't,
        then this will attempt to truncate the name and combine it with the original name
        to add randomness.\n
        If that criteria isn't met, then will attempt to call @c wxRenameFile.
        This can be called if a regular call to @c wxRenameFile fails.
    @param srcPath The original file to rename/move.
    @param destPath The destination name.
    @returns @c true if successful.*/
bool RenameFileShortenName(const wxString& srcPath, const wxString& destPath);
/** @returns A shortened version of a filepath.
    @param filePath The filepath to shorten.
    @param maxLength The maximum length of the shortened name.*/
[[nodiscard]]
wxString GetShortenedFilePath(const wxString& filePath, size_t maxLength = 40);
/// @brief Strips illegal characters from a file path, except for path separators.
/// @param filePath The path to strip.
/// @returns The stripped path.
[[nodiscard]]
wxString StripIllegalFileCharacters(const wxString& filePath);
/** @returns A usable title from a filepath/Url.\n
        This function will clean up the file name as best possible to make
        it a working title/filename for a project.
    @param filename The file path to get the title from.*/
[[nodiscard]]
wxString ParseTitleFromFileName(wxString filename);
/// @brief Takes a full file path and tries to find it in a new folder system,
///     using the folder structure of the original file.
/// @details This is useful when you have a Windows file path and need to find
///     it on a UNIX system (assuming the paths are still relative).
/// @returns The matching file path, or empty string if not found.
/// @param currentDir The folder to start the search from.
/// @param fileToFind The file name to search for.
[[nodiscard]]
wxString FindFileInMatchingDirStructure(const wxString& currentDir, const wxString& fileToFind);
/** @brief Sends a file to the recycle bin.
    @param fileToDelete The file to delete.
    @returns @c true on success.
    @todo Currently only works on MSW; other platforms will just permanently delete the file.*/
bool SendToRecycleBinOrDelete(const wxString& fileToDelete);
/** @brief Compares two files, based on the OS's filename case sensitivity.
    @param file1 The first file.
    @param file2 The second file.
    @returns The comparison result.*/
int CompareFilePaths(const wxString& file1, const wxString& file2);
/// @brief Retrieves all the subdirectories within a given directory.
/// @param rootDirectory The directory to list subdirectories from.
/// @param[out] subDirs The paths of the subdirectories in the main folder.
/// @returns @c -1 if the folder can't be transversed,
///     or the number of subdirectories upon success.
int GetAllDirs(const wxString& rootDirectory, wxArrayString& subDirs);
/** @brief Filters an array of file paths to only include files with
        extensions from a file filter.
    @param files The file array to filter.
    @param fileExtensions The file filter to filter with.
    @returns The files that match the extension patterns.*/
[[nodiscard]]
wxArrayString FilterFiles(const wxArrayString& files, const wxString& fileExtensions);
/** @returns An array of strings as a folder structure.
    @param dirs The folder names to combine.*/
[[nodiscard]]
wxString JoinDirs(const wxArrayString& dirs);
/** @returns An array of strings as a web folder structure.
    @param dirs The folder names to combine.*/
[[nodiscard]]
wxString JoinWebDirs(const wxArrayString& dirs);
/** @returns The list of extensions inside a file filter string.
    @param fileFilter The file filter.*/
[[nodiscard]]
wxString ExtractExtensionsFromFileFilter(const wxString& fileFilter);
/** @brief Deletes empty folder from top-level folder, recursively.
    @param rootDirectory The root directory.
    @returns @c true if deletions (if any) were successful.*/
bool RemoveEmptyDirsRecursively(const wxString& rootDirectory);
/** @brief Combines a folder and file (and possible proceeding folder)
        into a full path.
    @param directoryToCombineWith The root directory.
    @param fileOrFolderToCombine The file to added.
    @param[out] newPath The final path.
    @returns @c true if successful.*/
bool PathCombine(const wxString& directoryToCombineWith, const wxString& fileOrFolderToCombine,
                 wxString& newPath);
/// @brief Moves a directory and its files.
/// @param fromDirectory The folder to move.
/// @param toDirectory Where to move the folder to.
/// @returns @c true if successful.
/// @todo: currently fails to copy over empty folders
bool MoveDirectory(const wxString& fromDirectory, const wxString& toDirectory);

/** @brief Creates a new file based on @c filePath, embedding a numeric
            sequence in it (making it unique).
            This is useful for saving a file and not overwriting one that
            already exists with the same name.
        @param filePath The original filepath
        @returns The new filepath that was created.*/
[[nodiscard]]
wxString CreateNewFileName(const wxString& filePath);

/** @returns The extension (or simply domain) from an URL.
    @param url The URL to parse.*/
[[nodiscard]]
wxString GetExtensionOrDomain(const wxString& url);

/** @returns The last common folder between two paths and the position where it was found.\n
        An empty string and `wxString::npos` will be returned if no match can be found.
    @param path1 The first path to compare.
    @param path2 The second path to compare.
    @warning This assumes that both paths are using the same
        (and consistent) path separators; otherwise, `/` and `\`
        are both supported.*/
[[nodiscard]]
std::pair<wxString, size_t> GetCommonFolder(const wxString& path1, const wxString& path2);

/** @}*/

#endif // FILE_UTIL_H
