//////////////////////////////////////////////////////////////////////
// Name:        fileutil.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "fileutil.h"

//------------------------------------------------
wxString FilePathResolverBase::ResolvePath(const wxString& path,
    std::initializer_list<wxString> pathsToSearch /*= std::initializer_list<wxString>{}*/)
    {
    // reset
    m_fileType = FilePathType::InvalidFileType;
    m_path.clear();

    if (path.empty())
        { return m_path; }
    m_path = path;
    m_path.Trim(false);
    m_path.Trim(true);

    // see if it is a web file
    if (string_util::strnicmp(m_path.wc_str(), _DT(L"http:"), 5) == 0)
        {
        // fix Windows forwards slashes (which is wrong in an URL)
        m_path.Replace(L"\\", L"/");
        m_path.Replace(L" ", L"%20");

        m_fileType = FilePathType::HTTP;
        return m_path;
        }
    else if (string_util::strnicmp(m_path.wc_str(), _DT(L"https:"), 6) == 0)
        {
        m_path.Replace(L"\\", L"/");
        m_path.Replace(L" ", L"%20");

        m_fileType = FilePathType::HTTPS;
        return m_path;
        }
    else if (string_util::strnicmp(m_path.wc_str(), _DT(L"ftp:"), 4) == 0)
        {
        m_path.Replace(L"\\", L"/");
        m_fileType = FilePathType::FTP;
        return m_path;
        }
    else if (string_util::strnicmp(m_path.wc_str(), _DT(L"ftps:"), 5) == 0)
        {
        m_path.Replace(L"\\", L"/");
        m_fileType = FilePathType::FTPS;
        return m_path;
        }
    else if (string_util::strnicmp(m_path.wc_str(), _DT(L"gopher:"), 7) == 0)
        {
        m_path.Replace(L"\\", L"/");
        m_fileType = FilePathType::Gopher;
        return m_path;
        }
    // not a protocol, but just in case the protocol was forgotten
    else if (string_util::strnicmp(m_path.wc_str(), _DT(L"www."), 4) == 0)
        {
        m_path.Replace(L"\\", L"/");
        m_path.Replace(L" ", L"%20");
        m_path.insert(0, L"http://"); // safe assumption to fallback to

        m_fileType = FilePathType::HTTP;
        return m_path;
        }
    // else, if a file path using the file protocol then strip off the protocol
    else if (string_util::strnicmp(m_path.wc_str(), L"file:", 5) == 0)
        {
        if (string_util::strnicmp(m_path.wc_str(), L"file://localhost/", 17) == 0)
            { m_path = m_path.substr(17); }
        else if (string_util::strnicmp(m_path.wc_str(), L"file:///", 8) == 0)
            {
            m_path = m_path.substr(8);
            m_path.Replace(L"%20", L" ");
            }
        m_fileType = FilePathType::LocalOrNetwork;
        return m_path;
        }
    // Otherwise, see if the file exists locally or on a network (e.g., a UNC path).
    else if (HasLocalOrNetworkPrefix(GetResolvedPath()))
        {
        FilePathType specificLocalType = FilePathType::LocalOrNetwork;

        // if not found, see if it might be file inside of an archive or a cell in a spreadsheet.
        // a heuristic check to see if it at least is a real file path will then will use the special
        // file type that we determine here.
        wxRegEx RE(L"[.](xlsx|zip)#", wxRE_ICASE);
        if (RE.Matches(GetResolvedPath()))
            {
            size_t start(0), len(0);
            if (RE.GetMatch(&start, &len, 0))
                {
                auto extMatch = GetResolvedPath().substr(start, len);
                if (extMatch.CmpNoCase(_DT(L".xlsx#")) == 0)
                    { specificLocalType = FilePathType::ExcelCell; }
                else if (extMatch.CmpNoCase(_DT(L".zip#")) == 0)
                    { specificLocalType = FilePathType::ArchivedFile; }
                }
            }
        if (!wxFile::Exists(m_path) )
            {
            // see if this is a local file (that just doesn't exist) by seeing if
            // it has a drive in front of it
        #ifdef __WXMSW__
            if (HasWindowsPrefix(GetResolvedPath()))
                {
                m_fileType = specificLocalType;
                // CSIDL_PROGRAM_FILES will actually return "(x86)" if called from a 32-bit program,
                // so need to hard code these strings
                const wxString programFilesFolder = L"C:\\Program Files\\";
                const wxString programFilesFolder86 = L"C:\\Program Files (x86)\\";
                // see if changing "program files" to "program files (x86)" will find the file
                if (m_path.length() > programFilesFolder.length() &&
                    m_path.substr(0, programFilesFolder.length()).CmpNoCase(programFilesFolder) == 0)
                    {
                    const wxString currentNewPath = programFilesFolder86 +
                        m_path.substr(programFilesFolder.length());
                    if (wxFileName::FileExists(currentNewPath))
                        { m_path = currentNewPath; }
                    }
                // or vice versa
                else if (m_path.length() > programFilesFolder86.length() &&
                    m_path.substr(0, programFilesFolder86.length()).CmpNoCase(programFilesFolder86) == 0)
                    {
                    const wxString currentNewPath = programFilesFolder +
                        m_path.substr(programFilesFolder86.length());
                    if (wxFileName::FileExists(currentNewPath))
                        { m_path = currentNewPath; }
                    }
                return m_path;
                }
            // if a UNIX file path on a Windows system or UNC that
            // couldn't be found, then assume it's OK
            else
                {
                m_fileType = specificLocalType;
                return m_path;
                }
        #else
            // on UNIX, just assume file path is legit if the prefix checked out
            m_fileType = specificLocalType;
            // but do fix any forward slashes (Windows format)
            m_path.Replace(L"\\", L"/", true);
            // chop off Windows drive letter
            if (HasWindowsPrefix(GetResolvedPath()) )
                { m_path.erase(0,2); }
            return m_path;
        #endif
            }
        else
            {
            m_fileType = specificLocalType;
            return m_path;
            }
        }
    else
        {
        // see if in other provided paths
        for (const auto& otherPath : pathsToSearch)
            {
            if (const auto absPath{ wxFileName(m_path).GetAbsolutePath(otherPath) };
                wxFile::Exists(absPath))
                {
                m_path = absPath;
                m_fileType = FilePathType::LocalOrNetwork;
                return m_path;
                }
            }
        // ...or in the CWD
        if (const auto absPath{ wxFileName(m_path).GetAbsolutePath() };
            wxFile::Exists(absPath))
            {
            m_path = absPath;
            m_fileType = FilePathType::LocalOrNetwork;
            return m_path;
            }

        m_fileType = FilePathType::InvalidFileType;
        return m_path;
        }
    }

//------------------------------------------------
bool FilePathResolverBase::HasWindowsPrefix(const wxString& str)
    {
    return (str.length() >= 3 &&
            ((str[0] >= L'a' && str[0] <= L'z') ||
             (str[0] >= L'A' && str[0] <= L'Z')) &&
            str[1] == L':' &&
            string_util::is_either<wchar_t>(str[2], L'\\', L'/') );
    }

//------------------------------------------------
bool FilePathResolverBase::HasUnixPrefix(const wxString& str)
    {
    return (str.length() >= 2 &&
            str[0] == L'/' &&
            ((str[1] >= L'a' && str[1] <= L'z') ||
             (str[1] >= L'A' && str[1] <= L'Z')));
    }

//------------------------------------------------
bool FilePathResolverBase::HasNetworkPrefix(const wxString& str)
    {
    return (str.length() >= 2 &&
            str[0] == L'\\' &&
            str[1] == L'\\');
    }

//------------------------------------------------
wxString ParseTitleFromFileName(wxString filename)
    {
    // if page is just a PHP query, then use the name of the folder
    if (wxFileName(filename).GetName().StartsWith(L"?") )
        { filename = wxFileName(filename).GetPath(); }
    // sometimes webpage paths end with a '/', so chop that off when getting the title
    if (filename.EndsWith(L"/") )
        { filename = filename.substr(0, filename.length()-1); }
    FilePathResolverBase resolvePath(filename);
    filename = resolvePath.GetResolvedPath();
    // paths to worksheet/cell inside of Excel file should keep the spreadsheet file extension
    if (resolvePath.IsExcelCell())
        { filename.Replace(L".", wxString{}, true); }
    wxString retVal = StripIllegalFileCharacters(wxFileName(filename).GetName());
    retVal.Replace(L".", wxString{}, true);
    return retVal;
    }

//------------------------------------------------
wxString FindFileInMatchingDirStructure(const wxString& currentDir, const wxString& fileToFind)
    {
    if (currentDir.empty() || fileToFind.empty())
        { return wxString{}; }

    // get the file name from the path (which may be in a foreign OS file path format)
    FilePathResolverBase pathResolve(fileToFind);
    wxFileName filePath(pathResolve.GetResolvedPath());

    // just see if the file is in the current directory
    if (wxFileName::FileExists(currentDir + wxFileName::GetPathSeparator() + filePath.GetFullName()))
        { return currentDir + wxFileName::GetPathSeparator() + filePath.GetFullName(); }

    // convert the file structure to a different platform's structure (e.g., macOS to Windows)
        {
        wxArrayString originalDirSystem = filePath.GetDirs();

        // piece together the new directory with the old path until we come up with a found file
        while (originalDirSystem.GetCount())
            {
            const wxString currentNewPath = currentDir + wxFileName::GetPathSeparator() +
                JoinDirs(originalDirSystem) + filePath.GetFullName();
            if (wxFileName::FileExists(currentNewPath))
                { return currentNewPath; }
            originalDirSystem.RemoveAt(0);
            }
        }
    // or see if file being searched for is in a subdirectory of the current directory
        {
        wxArrayString subDirs;
        GetAllDirs(currentDir, subDirs);
        for (size_t i = 0; i < subDirs.size(); ++i)
            {
            const wxString currentNewPath = subDirs[i] + wxFileName::GetPathSeparator() +
                filePath.GetFullName();
            if (wxFileName::FileExists(currentNewPath))
                { return currentNewPath; }
            }
        }
    // or go up out of the current directory
        {
        wxArrayString originalDirSystem = wxFileName(currentDir).GetDirs();

        // piece together the new directory with the old path until we come up with a found file
        while (originalDirSystem.GetCount())
            {
            const wxString currentNewPath =
        #ifdef __WXMSW__
                wxFileName(currentDir).GetVolume() + wxFileName::GetVolumeSeparator() +
                wxFileName::GetPathSeparator() +
        #else
                L"/" +
        #endif
            JoinDirs(originalDirSystem) + filePath.GetFullName();
            if (wxFileName::FileExists(currentNewPath))
                { return currentNewPath; }
            originalDirSystem.RemoveAt(originalDirSystem.GetCount()-1);
            }
        }
    // couldn't be found
    return wxString{};
    }

//------------------------------------------------
wxString GetShortenedFileName(const wxString& filePath, const size_t maxLength /*= 40*/)
    {
    // if the path is shorter than the max allowed length, just return it
    if (filePath.length() <= maxLength)
        { return filePath; }
    else
        {
        FilePathResolverBase resolver(filePath);
        if (resolver.IsLocalOrNetworkFile())
            {
            wxFileName fn(filePath);
            wxArrayString astrTemp = fn.GetDirs();
            wxString volumePath = fn.GetVolume() + fn.GetVolumeSeparator() + fn.GetPathSeparator();

            /* let's replace each part with ellipsis, until the length is OK
               (but never substitute drive and file name)*/
            for (size_t i = 0; i < astrTemp.GetCount(); ++i)
                {
                // if the lengths for the directories can hold more than just ellipses
                // then try to include their names
                if ((volumePath.length() + JoinDirs(astrTemp).length() + fn.GetFullName().length()) <=
                    maxLength)
                    { break; }
                else
                    { astrTemp[i] = wxString(1,wchar_t(8230)); }
                }
            return volumePath + JoinDirs(astrTemp) + fn.GetFullName();
            }
        else if (resolver.IsHTTPFile() || resolver.IsHTTPSFile())
            {
            size_t slash = filePath.find(L'/');
            if (slash == wxString::npos || slash == filePath.length()-1)
                { return filePath; }
            if (filePath[slash+1] == L'/') // skip the "http://"
                { slash = filePath.find(L'/', slash+2); }
            if (slash == wxString::npos || slash == filePath.length()-1)
                { return filePath; }
            const size_t lastSlash = filePath.find_last_of(L'/');
            if (lastSlash == wxString::npos || lastSlash <= slash || lastSlash == filePath.length()-1)
                { return filePath; }
            wxString domain = filePath.substr(0, slash);
            wxString fileName = filePath.substr(lastSlash+1);
            wxString foldersString = filePath.substr(slash+1, (lastSlash-slash)-1);

            wxArrayString folders;
            wxStringTokenizer tkz(foldersString, L'/');
            wxString nextFileExt;
            while ( tkz.HasMoreTokens() )
                { folders.Add(tkz.GetNextToken()); }
            for (size_t i = 0; i < folders.GetCount(); ++i)
                {
                // if the lengths for the directories can hold more than just ellipses
                // then try to include their names
                if ((domain.length() + JoinWebDirs(folders).length() + fileName.length()) <=
                    maxLength)
                    { break; }
                else
                    { folders[i] = wxString(1,wchar_t(8230)); }
                }

            return domain + L'/' + JoinWebDirs(folders) + fileName;
            }
        else
            { return filePath; }
        }
    }

//------------------------------------------------
wxString StripIllegalFileCharacters(const wxString& filePath)
    {
    wxString strippedFilePath = filePath;
    const wxString forbiddenStrings = wxFileName::GetForbiddenChars();
    for (size_t i = 0; i < forbiddenStrings.length(); ++i)
        {
        if (forbiddenStrings[i] != wxFileName::GetPathSeparator())
            { strippedFilePath.Replace(wxString(forbiddenStrings[i]), wxString{}, true); }
        }
    for (size_t i = 0; i < strippedFilePath.length(); /*in loop*/)
        {
        if (strippedFilePath[i] < 32)
            { strippedFilePath.erase(i, 1); }
        else
            { ++i; }
        }
    strippedFilePath.Trim(false);
    strippedFilePath.Trim(true);
    return strippedFilePath;
    }

//------------------------------------------------------------------
bool SendToRecycleBinOrDelete(const wxString& fileToDelete)
    {
    if (!wxFile::Exists(fileToDelete) )
        { return false; }
#ifdef __WXMSW__
    /* file path needs to have TWO null terminators for SHFileOperation,
       so we need to use a different filepath buffer with two NULLs at the end.*/
    const size_t fileBufferLength = fileToDelete.length() + 2;
    auto filePath = std::make_unique<wchar_t[]>(fileBufferLength);
    std::wcsncpy(filePath.get(), fileToDelete.c_str(), fileToDelete.length());

    SHFILEOPSTRUCT SHFileOp;
    std::memset(&SHFileOp, 0, sizeof(SHFILEOPSTRUCT));
    SHFileOp.wFunc = FO_DELETE;
    SHFileOp.pFrom = filePath.get();
    SHFileOp.fFlags = FOF_ALLOWUNDO;

    // SHFileOperation returns 0 on success, so negate it
    return !::SHFileOperation(&SHFileOp);
#else
    return wxRemoveFile(fileToDelete);
#endif
    }

//------------------------------------------------------------------
int GetAllDirs(const wxString& rootDirectory, wxArrayString& subDirs)
    {
    wxDir dir(rootDirectory);

    if (!dir.IsOpened() )
        { return -1; }

    wxString filename;

    int counter = 0;
    bool cont = dir.GetFirst(&filename, wxString{}, wxDIR_DIRS|wxDIR_HIDDEN);
    while (cont)
        {
        ++counter;

        wxString subdir = rootDirectory;
        if (subdir.length() &&
            subdir.at(subdir.length() - 1) != wxFileName::GetPathSeparator())
            { subdir += wxFileName::GetPathSeparator(); }
        subdir += filename;

        subDirs.Add(subdir);
        const auto countersubs = GetAllDirs(subdir, subDirs);
        if (countersubs > 0)
            { counter += countersubs; }
        cont = dir.GetNext(&filename);
        }
    return counter;
    }

//------------------------------------------------------------------
wxString ExtractExtensionsFromFileFilter(const wxString& fileFilter)
    {
    wxString retVal = fileFilter;
    // get the actual filter inside of the "()" section of the string
    auto index = retVal.find(L'(');
    if (index != wxString::npos)
        { retVal.erase(0, index + 1); }
    index = retVal.find(L')', true);
    if (index != wxString::npos)
        { retVal.Truncate(index); }
    if (retVal == L"*.*")
        { retVal = wxFileSelectorDefaultWildcardStr; }
    return retVal;
    }

//------------------------------------------------------------------
wxString JoinDirs(const wxArrayString& dirs)
    {
    wxString fullPath;
    for (size_t i = 0; i < dirs.GetCount(); ++i)
        {
        fullPath += dirs[i] + wxFileName::GetPathSeparator();
        }
    return fullPath;
    }

//------------------------------------------------------------------
wxString JoinWebDirs(const wxArrayString& dirs)
    {
    wxString fullPath;
    for (size_t i = 0; i < dirs.GetCount(); ++i)
        { fullPath += dirs[i] + L'/'; }
    return fullPath;
    }

//------------------------------------------------------------------
wxArrayString FilterFiles(const wxArrayString& files, const wxString& fileExtensions)
    {
    // if using "all files" wildcard then don't bother filtering
    if (fileExtensions.Cmp(wxFileSelectorDefaultWildcardStr) == 0)
        { return files; }
    wxArrayString matchedFiles;
    matchedFiles.reserve(files.size());
    std::set<wxString, Wisteria::Data::wxStringLessNoCase> validExtensions;
    wxStringTokenizer tkz(fileExtensions, L"*.;");
    wxString nextFileExt;
    while ( tkz.HasMoreTokens() )
        {
        nextFileExt = tkz.GetNextToken();
        if (!nextFileExt.empty())
            { validExtensions.insert(nextFileExt); }
        }
    for (const auto& file : files)
        {
        if (validExtensions.find(wxFileName(file).GetExt()) != validExtensions.cend())
            { matchedFiles.push_back(file); }
        }
    return matchedFiles;
    }

//------------------------------------------------------------------
bool RemoveEmptyDirsRecursively(const wxString& rootDirectory)
    {
    wxString rdir = rootDirectory;
    if (rootDirectory.length() &&
        rootDirectory.at(rootDirectory.length() - 1) != wxFileName::GetPathSeparator())
        { rdir += wxFileName::GetPathSeparator(); }
    wxArrayString subDirs;
    const auto numberOfDirs = GetAllDirs(rdir, subDirs);
    if (numberOfDirs == -1)
        { return false; }
    else if (numberOfDirs == 0)
        { return wxFileName::Rmdir(rdir); }
    // reverse order to make the longer paths at the top
    subDirs.Sort(true);

    for (size_t i = 0; i < subDirs.GetCount(); ++i)
        { wxFileName::Rmdir(subDirs[i]); }

    wxFileName::Rmdir(rdir);
    return true;
    }

//------------------------------------------------------------------
bool PathCombine(const wxString& directoryToCombineWith,
                 const wxString& fileOrFolderToCombine, wxString& newPath)
    {
    newPath.Clear();

    wxFileName fileOrFolderToCombineName = fileOrFolderToCombine;
    wxFileName directoryToCombineWithName = directoryToCombineWith;

    /* If we have volumes (e.g., Windows's drive letters) then see if they are the same.
       If they are different, then just chop off the volume from the filename being combined
       and append it to the destination folder.*/
    if (fileOrFolderToCombineName.HasVolume() && directoryToCombineWithName.HasVolume())
        {
        wxString fullVolumeName = directoryToCombineWithName.GetVolume() +
                                  directoryToCombineWithName.GetVolumeSeparator() +
                                  directoryToCombineWithName.GetPathSeparator();
        if (fileOrFolderToCombineName.GetVolume().CmpNoCase(directoryToCombineWithName.GetVolume()) != 0)
            {
            newPath = directoryToCombineWith;
            newPath += fileOrFolderToCombineName.GetFullName().substr(fullVolumeName.length());
            }
        }

    wxArrayString fileOrFolderToCombineDirs = fileOrFolderToCombineName.GetDirs();
    wxArrayString directoryToCombineDirs = directoryToCombineWithName.GetDirs();

    size_t i = 0;
    for (i = 0;
        i < fileOrFolderToCombineName.GetDirCount() && i < directoryToCombineWithName.GetDirCount();
        ++i)
        {
        if (fileOrFolderToCombineDirs[i].CmpNoCase(directoryToCombineDirs[i]) != 0)
            { break; }
        }

    if (i < fileOrFolderToCombineName.GetDirCount())
        {
        newPath = directoryToCombineWithName.GetFullPath();
        for (; i < fileOrFolderToCombineName.GetDirCount(); ++i)
            { directoryToCombineWithName.AppendDir(fileOrFolderToCombineDirs[i]); }
        newPath = directoryToCombineWithName.GetFullPath();
        }

    directoryToCombineWithName.SetFullName(fileOrFolderToCombineName.GetFullName());
    newPath = directoryToCombineWithName.GetFullPath();

    return true;
    }

//------------------------------------------------------------------
int CompareFilePaths(const wxString& file1, const wxString& file2)
    { return wxFileName::IsCaseSensitive() ? file1.Cmp(file2) : file1.CmpNoCase(file2); }

//------------------------------------------------------------------
bool MoveDirectory(const wxString& fromDirectory, const wxString& toDirectory)
    {
    /* don't allow a parent directory to be copied into one of its subfolders,
       or let a folder be moved to itself*/
    if (wxStrnicmp(fromDirectory, toDirectory, fromDirectory.length()) == 0)
        { return false; }

    wxString newFileName;
    wxString fromDir = fromDirectory;
    wxString toDir = toDirectory;
    if (fromDir.length() &&
        fromDir.at(fromDir.length() - 1) != wxFileName::GetPathSeparator())
        { fromDir += wxFileName::GetPathSeparator(); }
    if (toDir.length() &&
        toDir.at(toDir.length() - 1) != wxFileName::GetPathSeparator())
        { toDir += wxFileName::GetPathSeparator(); }
    // see how much we need to trim off of file paths to get the relative paths
    wxFileName newFolder = fromDir;
    long rootFolderPathLength = 0;
    if (newFolder.GetDirCount())
        {
        rootFolderPathLength = static_cast<long>(newFolder.GetPath().length());
        rootFolderPathLength -=
            static_cast<long>(newFolder.GetDirs()[newFolder.GetDirCount()-1].length());
        }
    // no directory count? something is wrong, so bail
    else
        { return false; }

    wxDir dir;
    wxArrayString filesToMove;
    const size_t numberOfFiles = dir.GetAllFiles(fromDir, &filesToMove);
    if (numberOfFiles == 0)
        {
        newFolder = fromDir;
        if (newFolder.GetDirCount() &&
            PathCombine(toDir, newFolder.GetDirs()[newFolder.GetDirCount()-1], newFileName))
            {
            wxFileName::Mkdir(newFileName, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
            // recursively empty folder
            RemoveEmptyDirsRecursively(fromDirectory);
            }
        else
            { return false; }
        }

    wxProgressDialog progressDlg(wxString::Format(_(L"Moving %s"), fromDirectory),
        _(L"Moving Folder"),
        static_cast<int>(filesToMove.GetCount()), nullptr,
        wxPD_AUTO_HIDE|wxPD_SMOOTH|wxPD_ESTIMATED_TIME|wxPD_REMAINING_TIME|wxPD_CAN_ABORT);
    progressDlg.Centre();
    progressDlg.Raise();
    for (size_t i = 0; i < filesToMove.GetCount(); ++i)
        {
        wxYield();
        if (!progressDlg.Update(static_cast<int>(i),
            wxString::Format(_(L"Moving %s"), filesToMove[i])) )
            { return false; }
        wxString relativeFilePath = filesToMove[i];
        relativeFilePath.Truncate(rootFolderPathLength);
        if (PathCombine(toDir, relativeFilePath, newFileName))
            {
            newFolder = newFileName;
            if (!newFolder.DirExists(newFolder.GetPath()))
                { newFolder.Mkdir(newFolder.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL); }
            wxRenameFile(filesToMove[i], newFileName);
            }
        }

    RemoveEmptyDirsRecursively(fromDirectory);

    return true;
    }
