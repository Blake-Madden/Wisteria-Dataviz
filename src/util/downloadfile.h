/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2023
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
#include "../import/html_extract_text.h"

/** @brief Queues a list of URLs and their respective (local) download paths
        and then downloads them asynchronously.
    @par Example
        An `wxEvtHandler`-derived class (a @c wxFrame, @c wxApp, etc.)
        should store an initialize a @c QueueDownload object as a member
        and then initialize it as such:
    @code
        // You can also call SetEventHandler() and bind wxEVT_WEBREQUEST_STATE and
        // wxEVT_WEBREQUEST_DATA yourself if you prefer; this is a shortcut for that.
        m_downloader.SetAndBindEventHandler(this);

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
    @warning An `wxEvtHandler`-derived class can either be connected to a single QueueDownload
        or a single FileDownload object. This is because the class must bind its @c wxEVT_WEBREQUEST_STATE
        event to the `QueueDownload`'s or `FileDownload`'s @c ProcessRequest() method.
*/
class QueueDownload
    {
public:
    /// @brief Constructor.
    /// @param handler The parent event handler (e.g., dialog or @c wxApp)
    ///     to connect to this queue.
    explicit QueueDownload(wxEvtHandler* handler) : m_handler(handler)
        {};
    /// @private
    QueueDownload() = default;
    /// @private
    QueueDownload(const QueueDownload&) = delete;
    /// @private
    QueueDownload& operator=(const QueueDownload&) = delete;

    /// @brief Connect the download queue to a parent dialog or @c wxApp.
    /// @param handler The @c wxEvtHandler to connect the queue to.
    /// @sa ProcessRequest(), SetAndBindEventHandler().
    void SetEventHandler(wxEvtHandler* handler)
        { m_handler = handler; }
    /// @brief Connect the downloader to a parent dialog or @c wxApp, and also
    ///     bind the event handler's @c wxEVT_WEBREQUEST_STATE and @c wxEVT_WEBREQUEST_DATA
    ///     events to this object.
    /// @param handler The @c wxEvtHandler to connect the downloader to.
    /// @note It is recommended to call @c CancelPending() in the event handler's
    ///     close event (that will not be bound here).
    void SetAndBindEventHandler(wxEvtHandler* handler)
        {
        m_handler = handler;
        m_handler->Bind(wxEVT_WEBREQUEST_STATE, &QueueDownload::ProcessRequest, this);
        m_handler->Bind(wxEVT_WEBREQUEST_DATA, &QueueDownload::ProcessRequest, this);
        }
    /** @brief Sets the user agent to send the server when connecting.
        @param userAgent The user agent to use.*/
    void SetUserAgent(wxString userAgent)
        { m_userAgent = std::move(userAgent); }
    [[nodiscard]]
    /// @returns The user agent being sent when connecting.
    const wxString& GetUserAgent() const noexcept
        { return m_userAgent; }
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
    ///     // assuming m_downloads is a QueueDownload member of the dialog
    ///     Bind(wxEVT_WEBREQUEST_STATE,
    ///          &QueueDownload::ProcessRequest, &m_downloads);
    /// @endcode
    void ProcessRequest(wxWebRequestEvent& evt);
    /// @brief Bind this to the parent `wxEvtHandler`'s close event
    ///     to close any downloads that still pending.
    void CancelPending();

    /** @brief Disable SSL certificate verification.
        @details This can be used to connect to self signed servers or other invalid SSL connections.\n
            Disabling verification makes the communication insecure.
        @param disable @c true to disable SSL certificate verification.*/
    void DisablePeerVerify(const bool disable)noexcept
        {
        m_disablePeerVerify = disable;
        }
    /// @returns Returns @c true if peer verification has been disabled.
    [[nodiscard]]
    bool IsPeerVerifyDisabled() const noexcept
        {
        return m_disablePeerVerify;
        }

    /// @brief Converts a response code to a readable message.
    /// @param responseCode The web request response code.
    /// @returns The meaning of the response code.
    [[nodiscard]]
    static wxString GetResponseMessage(const int responseCode)
        {
        if (responseCode > 0 && responseCode < 300)
            {
            return _(L"Connection successful.");
            }
        switch (responseCode)
            {
        case 301:
            return _(L"Page has moved.");
        case 302:
            return _(L"Page was found, but under a different URL.");
        case 204:
            return _(L"Page not responding.");
        case 400:
            return _(L"Bad request.");
        case 401:
            return _(L"Unauthorized.");
        case 402:
            return _(L"Payment Required.");
        case 403:
            return _(L"Forbidden.");
        case 404:
            return _(L"Page not found.");
        case 500:
            return _(L"Internal Error.");
        case 501:
            return _(L"Not implemented.");
        case 502:
            return _(L"Service temporarily overloaded.");
        case 503:
            return _(L"Gateway timeout.");
        default:
            return _(L"Unknown connection error.");
            };
        }
    /// @brief Determines if a response code indicates a connection failure.
    /// @param responseCode The web request response code.
    /// @returns @c true if the response code indicates a connection failure.
    [[nodiscard]]
    inline constexpr static bool IsBadResponseCode(const int responseCode) noexcept
        {
        return (responseCode == 204 || responseCode == 400 || responseCode == 401 ||
                responseCode == 402 || responseCode == 403 || responseCode == 404 ||
                responseCode == 500 || responseCode == 501 || responseCode == 502 ||
                responseCode == 503 || responseCode == 0);
        }
private:
    wxString GetLocalPath(const int ID) const;
    void Remove(const int ID);
    wxEvtHandler* m_handler{ nullptr };
    mutable std::mutex m_mutex;
    std::map<int, wxString> m_downloads;
    std::vector<wxWebRequest> m_requests;
    wxString m_userAgent;
    int m_currentId{ 0 };
    bool m_disablePeerVerify{ false };
    };

/** @brief Reads or downloads a file synchronously.
    @par Example
        An `wxEvtHandler`-derived class (a @c wxFrame, @c wxApp, etc.)
        should store an initialize a @c FileDownload object as a member
        and then initialize it as such:
    @code
        // You can also call SetEventHandler() and bind wxEVT_WEBREQUEST_STATE and
        // wxEVT_WEBREQUEST_DATA yourself if you prefer; this is a shortcut for that.
        m_downloadFile.SetAndBindEventHandler(this);
    @endcode

    Later, the `wxEvtHandler`-derived class can call GetResponse(), Read(), or Download() as such:
    @code
        // get the content type of a page (without reading its full content)
        const wxString contentType = m_downloadFile.GetResponse(
            "https://github.com/wxWidgets/wxWidgets/blob/master/README-GIT.md")
            .GetHeader("Content-Type");

        // download a file locally
        m_downloadFile.Download("https://github.com/wxWidgets/wxWidgets/blob/master/README-GIT.md",
                                wxStandardPaths::Get().GetDocumentsDir() + "/readme.md");

        // then read a webpage and copy it into a wxString
        wxString webPageContent(m_downloadFile.Read("https://www.wxwidgets.org/") ?
                                &m_downloadFile.GetLastRead()[0] : wxString{});
    @endcode
    @warning An `wxEvtHandler`-derived class can either be connected to a single QueueDownload
        or a single FileDownload object. This is because the class must bind its
        @c wxEVT_WEBREQUEST_STATE event to the `QueueDownload`'s or
        `FileDownload`'s @c ProcessRequest() method.
*/
class FileDownload
    {
public:
    /// @brief Constructor.
    /// @param handler The parent event handler (e.g., dialog or @c wxApp)
    ///     to connect to this queue.
    explicit FileDownload(wxEvtHandler* handler) : m_handler(handler)
        {};
    /// @private
    FileDownload() = default;
    /// @private
    FileDownload(const QueueDownload&) = delete;
    /// @private
    FileDownload& operator=(const QueueDownload&) = delete;

    /// @brief Connect the downloader to a parent dialog or @c wxApp.
    /// @param handler The @c wxEvtHandler to connect the downloader to.
    /// @sa ProcessRequest(), SetAndBindEventHandler().
    void SetEventHandler(wxEvtHandler* handler)
        { m_handler = handler; }
    /// @brief Connect the downloader to a parent dialog or @c wxApp, and also
    ///     bind the event handler's @c wxEVT_WEBREQUEST_STATE and @c wxEVT_WEBREQUEST_DATA
    ///     events to this object.
    /// @param handler The @c wxEvtHandler to connect the downloader to.
    void SetAndBindEventHandler(wxEvtHandler* handler)
        {
        m_handler = handler;
        m_handler->Bind(wxEVT_WEBREQUEST_STATE, &FileDownload::ProcessRequest, this);
        m_handler->Bind(wxEVT_WEBREQUEST_DATA, &FileDownload::ProcessRequest, this);
        }
    /** @brief Sets the user agent to send the server when connecting.
        @param userAgent The user agent to use.*/
    void SetUserAgent(wxString userAgent)
        { m_userAgent = std::move(userAgent); }
    /// @returns The user agent being sent when connecting.
    [[nodiscard]]
    const wxString& GetUserAgent() const noexcept
        { return m_userAgent; }
    /// @brief If @c true, shows a progress dialog while downloading a file.
    /// @param show @c true to show the progress dialog.
    void ShowProgress(const bool show) noexcept
        { m_showProgress = show; }
    /** @brief Disable SSL certificate verification.
        @details This can be used to connect to self signed servers or other invalid SSL connections.\n
            Disabling verification makes the communication insecure.
        @param disable @c true to disable SSL certificate verification.*/
    void DisablePeerVerify(const bool disable)noexcept
        {
        m_disablePeerVerify = disable;
        }
    /// @returns Returns @c true if peer verification has been disabled.
    [[nodiscard]]
    bool IsPeerVerifyDisabled() const noexcept
        {
        return m_disablePeerVerify;
        }
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
    /// @brief Attempts to connect to an URL and load its response.
    /// @param url The URL to check. 
    /// @note This will not read or download the webpage, it will only get its response.\n
    ///     If using @c SetEventHandler() instead of @c SetAndBindEventHandler(),
    ///     this object's event handler will need to have its @c  wxEVT_WEBREQUEST_DATA event
    ///     bound to object if calling this.
    /// @sa GetLastStatus(), GetLastUrl(), GetLastContentType().
    void RequestResponse(const wxString& url);
    /// @returns The last status from a read, download, or response request.
    [[nodiscard]]
    int GetLastStatus() const noexcept
        { return m_lastStatus; };
    /// @returns The last status message from a read, download, or response request.
    [[nodiscard]]
    const wxString& GetLastStatusText() const noexcept
        { return m_lastStatusText; };
    /// @returns The last status info from a read, download, or response request.
    /// @note This is debug info, usually the contents of a redirected error page.
    [[nodiscard]]
    const wxString& GetLastStatusInfo() const noexcept
        { return m_lastStatusInfo; };
    /// @returns The last url (or possible redirect) from a read, download, or response request.
    [[nodiscard]]
    const wxString& GetLastUrl() const noexcept
        { return m_lastUrl; };
    /// @returns The last @c Content-Type from a read, download, or response request.
    [[nodiscard]]
    const wxString& GetLastContentType() const noexcept
        { return m_lastContentType; };
    /// @brief Bind this to a @c wxEVT_WEBREQUEST_STATE in the
    ///     parent @c wxEvtHandler.
    /// @param evt The event to process.
    /// @par Example:
    /// @code
    ///     // assuming m_downloadFile is a FileDownload member of the dialog
    ///     Bind(wxEVT_WEBREQUEST_STATE,
    ///          &FileDownload::ProcessRequest, &m_downloadFile);
    ///     Bind(wxEVT_WEBREQUEST_DATA,
    ///         &FileDownload::ProcessRequest, &m_downloadFile);
    /// @endcode
    void ProcessRequest(wxWebRequestEvent& evt);
private:
    std::vector<char> m_buffer;
    bool m_stillActive{ false };
    bool m_downloadSuccessful{ false };
    bool m_showProgress{ false };
    bool m_disablePeerVerify{ false };
    wxEvtHandler* m_handler{ nullptr };
    mutable std::mutex m_mutex;
    wxString m_downloadPath;
    wxString m_readContent;
    wxString m_userAgent;
    int m_lastStatus{ 404 };
    wxString m_lastStatusText;
    wxString m_lastUrl;
    wxString m_lastContentType;
    wxString m_lastStatusInfo;
    wxString m_server;
    wxWebRequest::State m_lastState{ wxWebRequest::State_Failed };
    };

/** @}*/

#endif // __DOWNLOADFILE_H__
