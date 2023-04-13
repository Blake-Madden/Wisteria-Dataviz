/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __DOWNLOADFILE_H__
#define __DOWNLOADFILE_H__

#include <wx/wx.h>
#include <wx/string.h>
#include <wx/log.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/webrequest.h>
#include <wx/creddlg.h>
#include <wx/progdlg.h>
#include <map>
#include <vector>
#include <mutex>

/** @brief Queues a list of URLs and their respective (local) download paths
        and then downloads them asynchronously.
    @par Example
        An `wxEvtHandler`-derived class (a @c wxFrame, @c wxApp, etc.)
        should store an initialize a @c DownloadQueue object as a member
        and then initialize it as such:
    @code
        m_downloader.SetEventHandler(this);

        // Bind state event
        Bind(wxEVT_WEBREQUEST_STATE, &DownloadQueue::ProcessRequest, &m_downloader);
        // either bind this, or call m_downloader.CancelPending() in the
        // wxEvtHandler's already-existing close event
        Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event)
            {
            m_downloader.CancelPending();
            event.Skip();
            });
    @endcode

    Later, the `wxEvtHandler`-derived class can queue and then download files as such:
    @code
        // Create the request objects, where we will download a few wxWidget
        // logos and save them to our Documents folder.
        m_downloader.Add("https://www.wxwidgets.org/downloads/logos/blocks.png",
                         wxStandardPaths::Get().GetDocumentsDir() + "/blocks.png");

        m_downloader.Add("https://www.wxwidgets.org/downloads/logos/powered-by-wxwidgets-88x31-blue.png",
                         wxStandardPaths::Get().GetDocumentsDir() + "/powered-by-wxwidgets-88x31-blue.png");

        m_downloader.Add("https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.1/wxWidgets-3.1.1.7z",
                         wxStandardPaths::Get().GetDocumentsDir() + "/title.png");

        // Start the requests, downloading the files asynchronously to their
        // respective local paths
        m_downloader.Start();
    @endcode
    @warning An `wxEvtHandler`-derived class can either be connected to a single DownloadQueue
        or a single DownloadFile object. This is because the class must bind its @c wxEVT_WEBREQUEST_STATE
        event to the `DownloadQueue`'s or `DownloadFile`'s @c ProcessRequest() method.
*/
class DownloadQueue
    {
public:
    /// @brief Constructor.
    /// @param handler The parent event handler (e.g., dialog or @c wxApp)
    ///     to connect to this queue.
    explicit DownloadQueue(wxEvtHandler* handler) : m_handler(handler)
        {};
    /// @private
    DownloadQueue() = default;
    /// @private
    DownloadQueue(const DownloadQueue&) = delete;
    /// @private
    DownloadQueue& operator=(const DownloadQueue&) = delete;

    /// @brief Connect the download queue to a parent dialog or @c wxApp.
    /// @param handler The @c wxEvtHandler to connect the queue to.
    void SetEventHandler(wxEvtHandler* handler)
        { m_handler = handler; }
    /// @brief Adds an URL and download path to the queue.
    /// @param url The web file to download.
    /// @param localDownloadPath Where to download to.
    void Add(const wxString& url, const wxString& localDownloadPath);
    /// @brief Start downloading the queued links.
    /// @note ProcessRequest() and CancelPending() should be bound
    ///     before calling this.
    void Start()
        {
        std::for_each(m_requests.begin(), m_requests.end(),
            [](auto& request)
            { request.Start(); });
        }
    /// @brief Bind this to a @c wxEVT_WEBREQUEST_STATE in the
    ///     parent @c wxEvtHandler.
    /// @param evt The event to process.
    /// @par Example:
    /// @code
    ///     // assuming m_downloads is a DownloadQueue member of the dialog
    ///     Bind(wxEVT_WEBREQUEST_STATE,
    ///          &DownloadQueue::ProcessRequest, &m_downloads);
    /// @endcode
    void ProcessRequest(wxWebRequestEvent& evt);
    /// @brief Bind this to the parent `wxEvtHandler`'s close event
    ///     to close any downloads that still pending.
    void CancelPending();
private:
    wxString GetLocalPath(const int ID) const;
    void Remove(const int ID);
    wxEvtHandler* m_handler{ nullptr };
    mutable std::mutex m_mutex;
    std::map<int, wxString> m_downloads;
    std::vector<wxWebRequest> m_requests;
    int m_currentId{ 0 };
    };

/** @brief Reads or downloads a file synchronously.
    @par Example
        An `wxEvtHandler`-derived class (a @c wxFrame, @c wxApp, etc.)
        should store an initialize a @c DownloadFile object as a member
        and then initialize it as such:
    @code
        m_downloadFile.SetEventHandler(this);

        // Bind state event
        Bind(wxEVT_WEBREQUEST_STATE, &DownloadFile::ProcessRequest, &m_downloadFile);
        // either bind this, or call m_downloadFile.CancelPending() in the
        // wxEvtHandler's already-existing close event
        Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event)
            {
            m_downloadFile.CancelPending();
            event.Skip();
            });
    @endcode

    Later, the `wxEvtHandler`-derived class can call Read or Download as such:
    @code
        // download a file locally
        m_downloadFile.Download("https://github.com/wxWidgets/wxWidgets/blob/master/README-GIT.md",
                                wxStandardPaths::Get().GetDocumentsDir() + "/readme.md");

        // then read a webpage and copy it into a wxString
        wxString webPageContent(m_downloadFile.Read("https://www.wxwidgets.org/") ?
                                &m_downloadFile.GetLastRead()[0] : wxString{});
    @endcode
    @warning An `wxEvtHandler`-derived class can either be connected to a single DownloadQueue
        or a single DownloadFile object. This is because the class must bind its @c wxEVT_WEBREQUEST_STATE
        event to the `DownloadQueue`'s or `DownloadFile`'s @c ProcessRequest() method.
*/
class DownloadFile
    {
public:
    /// @brief Constructor.
    /// @param handler The parent event handler (e.g., dialog or @c wxApp)
    ///     to connect to this queue.
    explicit DownloadFile(wxEvtHandler* handler) : m_handler(handler)
        {};
    /// @private
    DownloadFile() = default;
    /// @private
    DownloadFile(const DownloadQueue&) = delete;
    /// @private
    DownloadFile& operator=(const DownloadQueue&) = delete;

    /// @brief Connect the downloader to a parent dialog or @c wxApp.
    /// @param handler The @c wxEvtHandler to connect the downloader to.
    void SetEventHandler(wxEvtHandler* handler)
        { m_handler = handler; }
    /// @brief If @c true, shows a progress dialog while downloading a file.
    /// @param show @c true to show the progress dialog.
    void ShowProgress(const bool show) noexcept
        { m_showProgress = show; }
    /// @brief Downloads a web file to a local path.
    /// @param url The web file to download.
    /// @param localDownloadPath Where to download to.
    /// @returns @c true if download was successful.
    bool Download(const wxString& url, const wxString& localDownloadPath);
    /// @brief Reads the requested URL.
    /// @details This will be synchronous, so will not return until the
    ///     entire web file has been read.\n
    ///     Call GetLastRead() afterwards to get the web file's content.
    /// @param url The web file to download.
    /// @returns @c true if read was successful.
    /// @sa GetLastRead().
    bool Read(const wxString& url);
    /// @returns The read web file content from the last call to
    ///     Read(). This will be a @c char buffer that can be converted
    ///     into a wxString.
    const std::vector<char>& GetLastRead() const noexcept
        { return m_buffer; }
    /// @brief Bind this to a @c wxEVT_WEBREQUEST_STATE in the
    ///     parent @c wxEvtHandler.
    /// @param evt The event to process.
    /// @par Example:
    /// @code
    ///     // assuming m_downloadFile is a DownloadFile member of the dialog
    ///     Bind(wxEVT_WEBREQUEST_STATE,
    ///          &DownloadFile::ProcessRequest, &m_downloadFile);
    /// @endcode
    void ProcessRequest(wxWebRequestEvent& evt);
    /// @brief Bind this to the parent `wxEvtHandler`'s close event
    ///     to close any download or read that are still pending.
    /// @par Example:
    /// @code
    ///     // assuming m_downloadFile is a DownloadFile member of the dialog
    ///     Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event)
    ///         {
    ///         m_downloadFile.CancelPending();
    ///         event.Skip();
    ///         });
    /// @endcode
    /// @note If the parent @c wxEvtHandler already has a close event,
    ///     then you can simply call `m_downloadFile.CancelPending()` there.
    void CancelPending()
        {
        if (m_stillActive)
            { m_request.Cancel(); }
        }
private:
    std::vector<char> m_buffer;
    bool m_stillActive{ false };
    bool m_downloadSuccessful{ false };
    bool m_showProgress{ false };
    wxEvtHandler* m_handler{ nullptr };
    mutable std::mutex m_mutex;
    wxString m_downloadPath;
    wxString m_readContent;
    wxWebRequest m_request;
    };

/** @}*/

#endif // __DOWNLOADFILE_H__
