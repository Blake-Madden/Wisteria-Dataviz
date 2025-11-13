/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_DOWNLOADFILE_H
#define WISTERIA_DOWNLOADFILE_H

#include "../import/html_extract_text.h"
#include <chrono>
#include <map>
#include <mutex>
#include <optional>
#include <vector>
#include <wx/creddlg.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/string.h>
#include <wx/webrequest.h>
#include <wx/wx.h>

/** @brief Queues a list of URLs and their respective (local) download paths
        and then downloads them asynchronously.
    @par Example
        A `wxEvtHandler`-derived class (a @c wxFrame, @c wxApp, etc.)
        should store and initialize a @c QueueDownload object as a member
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
                         wxStandardPaths::Get().GetDocumentsDir() +
                         "/powered-by-wxwidgets-88x31-blue.png");

        m_downloader.Add("https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.1/wxWidgets-3.1.1.7z",
                         wxStandardPaths::Get().GetDocumentsDir() + "/title.png");

        // Start the requests, downloading the files asynchronously to their
        // respective local paths
        m_downloader.Start();
    @endcode
    @warning A `wxEvtHandler`-derived class can either be connected to a single QueueDownload
        or a single FileDownload object. This is because the class must bind its @c
   wxEVT_WEBREQUEST_STATE event to the `QueueDownload`'s or `FileDownload`'s @c ProcessRequest()
   method.
*/
class QueueDownload
    {
  public:
    /// @brief Constructor.
    /// @param handler The parent event handler (e.g., dialog or @c wxApp)
    ///     to connect to this queue.
    explicit QueueDownload(wxEvtHandler* handler) : m_handler(handler) {}

    /// @private
    QueueDownload() = default;
    /// @private
    QueueDownload(const QueueDownload&) = delete;
    /// @private
    QueueDownload& operator=(const QueueDownload&) = delete;

    /// @brief Connect the download queue to a parent dialog or @c wxApp.
    /// @param handler The @c wxEvtHandler to connect the queue to.
    /// @sa ProcessRequest(), SetAndBindEventHandler().
    void SetEventHandler(wxEvtHandler* handler) { m_handler = handler; }

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
    void SetUserAgent(const wxString& userAgent) { m_userAgent = userAgent; }

    [[nodiscard]]
    /// @returns The user agent being sent when connecting.
    const wxString& GetUserAgent() const noexcept
        {
        return m_userAgent;
        }

    /// @brief Adds an URL and download path to the queue.
    /// @param url The web file to download.
    /// @param localDownloadPath Where to download to.
    void Add(const wxString& url, const wxString& localDownloadPath);

    /// @brief Bind this to the parent `wxEvtHandler`'s close event
    ///     (or call from an existing close handler) to cancel
    ///     any download that still pending.
    void Start()
        {
        std::ranges::for_each(m_requests, [](auto& request) { request.Start(); });
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
    ///     to cancel any downloads that still pending.
    void CancelPending();

    /** @brief Disable SSL certificate verification.
        @details This can be used to connect to self-signed servers or
            other invalid SSL connections.\n
            Disabling verification makes the communication insecure.
        @param disable @c true to disable SSL certificate verification.*/
    void DisablePeerVerify(const bool disable) noexcept { m_disablePeerVerify = disable; }

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
        if (responseCode > 0 && responseCode < 300 && responseCode != 204)
            {
            return _(L"Connection successful");
            }
        switch (responseCode)
            {
        case 204:
            return _(L"Page not responding");
        case 301:
            return _(L"Page has moved");
        case 302:
            return _(L"Page was found, but under a different URL");
        case 400:
            return _(L"Bad request");
        case 401:
            return _(L"Unauthorized");
        case 402:
            return _(L"Payment Required");
        case 403:
            return _(L"Forbidden");
        case 404:
            return _(L"Page not found");
        case 500:
            return _(L"Internal Error");
        case 501:
            return _(L"Not implemented");
        case 502:
            return _(L"Service temporarily overloaded");
        case 503:
            return _(L"Gateway timeout");
        default:
            return _(L"Unknown connection error");
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

    /// @private
    constexpr static size_t KILOBYTE = 1024;
    /// @private
    constexpr static size_t MEGABYTE = 1024 * 1024;

  private:
    wxString GetLocalPath(int ID) const;
    void Remove(int ID);
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
        A `wxEvtHandler`-derived class (a @c wxFrame, @c wxApp, etc.)
        should store and initialize a @c FileDownload object as a member
        and then initialize it as such:
    @code
        // You can also call SetEventHandler() and bind wxEVT_WEBREQUEST_STATE and
        // wxEVT_WEBREQUEST_DATA yourself if you prefer; this is a shortcut for that.
        m_downloadFile.SetAndBindEventHandler(this);

        // either bind this, or call m_downloadFile.CancelPending() in the
        // wxEvtHandler's already-existing close event
        Bind(wxEVT_CLOSE_WINDOW, [this](wxCloseEvent& event)
            {
            m_downloadFile.CancelPending();
            event.Skip();
            });
    @endcode

    Later, the `wxEvtHandler`-derived class can call GetResponse(), Read(), or Download() as such:
    @code
        // get the content type from a page (without reading its full content)
        const wxString contentType = m_downloadFile.GetResponse(
            "https://github.com/wxWidgets/wxWidgets/blob/master/README-GIT.md")
            .GetHeader("Content-Type");

        // download a file locally
        m_downloadFile.Download("https://github.com/wxWidgets/wxWidgets/blob/master/README-GIT.md",
                                wxStandardPaths::Get().GetDocumentsDir() + "/readme.md");

        // then read a webpage and copy it into a string
        wxString webPageContent(m_downloadFile.Read("https://www.wxwidgets.org/") ?
                                m_downloadFile.GetLastRead().data() : wxString{});
    @endcode
    @warning A `wxEvtHandler`-derived class can either be connected to a single QueueDownload
        or a single FileDownload object. This is because the class must bind its
        @c wxEVT_WEBREQUEST_STATE event to the `QueueDownload`'s or
        `FileDownload`'s @c ProcessRequest() method.\n
        This also object is also not MT-safe, as its current state is synchronously bound
        to the event handler. In other words, this multiple FileDownload object's cannot be used
        to download (or read) pages at the same time if they are sharing the same event handler.
*/
class FileDownload
    {
  public:
    /// @brief Constructor.
    /// @param handler The parent event handler (e.g., dialog or @c wxApp)
    ///     to connect to this queue.
    explicit FileDownload(wxEvtHandler* handler) : m_handler(handler) {}

    /// @private
    FileDownload() = default;

    /// @private
    FileDownload(const QueueDownload&) = delete;

    /// @private
    FileDownload& operator=(const QueueDownload&) = delete;

    /// @brief Connect the downloader to a parent dialog or @c wxApp.
    /// @param handler The @c wxEvtHandler to connect the downloader to.
    /// @sa ProcessRequest(), SetAndBindEventHandler().
    void SetEventHandler(wxEvtHandler* handler) { m_handler = handler; }

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
    void SetUserAgent(const wxString& userAgent) { m_userAgent = userAgent; }

    /// @returns The user agent being sent when connecting.
    [[nodiscard]]
    const wxString& GetUserAgent() const noexcept
        {
        return m_userAgent;
        }

    /** @brief Sets the cookies to send the server when connecting.
        @param cookies The cookies to use.*/
    void SetCookies(const wxString& cookies) { m_cookies = cookies; }

    /// @returns The cookies being sent when connecting.
    [[nodiscard]]
    const wxString& GetCookies() const noexcept
        {
        return m_cookies;
        }

    /** @brief Disable SSL certificate verification.
        @details This can be used to connect to self-signed servers or other
            invalid SSL connections.\n
            Disabling verification makes the communication insecure.
        @param disable @c true to disable SSL certificate verification.*/
    void DisablePeerVerify(const bool disable) noexcept { m_disablePeerVerify = disable; }

    /// @returns Returns @c true if peer verification has been disabled.
    [[nodiscard]]
    bool IsPeerVerifyDisabled() const noexcept
        {
        return m_disablePeerVerify;
        }

    /// @brief Sets the minimum size that a file has to be to download it.
    /// @param size The minimum file size, in kilobytes.
    void SetMinimumDownloadFileSizeInKilobytes(const std::optional<uint32_t> size)
        {
        m_minFileDownloadSizeKilobytes = size;
        }

    /// @brief Sets the number of seconds before a request, read, or download will quit
    ///     due to inactivity.
    /// @param timeout The number of seconds to wait before timing out.
    void SetTimeout(const int timeout) noexcept { m_timeoutSeconds = timeout; }

    /// @returns The number of seconds before a request, read, or download will quit
    ///     due to inactivity.
    [[nodiscard]]
    int GetTimeout() const noexcept
        {
        return m_timeoutSeconds;
        }

    /// @brief Downloads a web file to a local path.
    /// @param url The web file to download.
    /// @param localDownloadPath Where to download to.
    /// @returns @c true if download was successful.
    bool Download(const wxString& url, const wxString& localDownloadPath);

    /// @brief Downloads the requested OneDrive file.
    /// @param url The OneDrive file to download.
    /// @param localDownloadFolder Folder to download to
    ///     (file name will be extracted from the OneDrive information).
    /// @returns @c true if download was successful.
    /// @note Call GetLastOneDriveFileName() afterwards to get the name of the downloaded file.
    bool DownloadOneDriveFile(const wxString& url, const wxString& localDownloadFolder);

    /// @brief Reads the requested URL.
    /// @details This will be synchronous, so will not return until the
    ///     entire web file has been read.\n
    ///     Call GetLastRead() afterward to get the web file's content.
    /// @param url The web file to download.
    /// @returns @c true if read was successful.
    /// @sa GetLastRead().
    bool Read(const wxString& url);

    /// @brief Reads the requested OneDrive file.
    /// @details This will be synchronous, so will not return until the
    ///     entire web file has been read.\n
    ///     Call GetLastRead() afterward to get the web file's content.
    /// @param url The web file to download.
    /// @returns @c true if read was successful.
    /// @sa GetLastRead(), GetLastOneDriveFileName().
    bool ReadOneDriveFile(const wxString& url);

    /** @brief If @c true, will use the filename sent from the server as the filename
            when downloading a file.
        @details This will override the filename (but not the folder path) sent
            to Download().\n
            The default is @c false, which will explicitly use the path sent
            by the client to Download().
        @param useSuggested Whether to use the suggested filename from the server.
        @sa GetDownloadPath() for retrieving the resolved download path after downloading.*/
    void UseSuggestedFileNames(const bool useSuggested) noexcept
        {
        m_useSuggestedFileName = useSuggested;
        }

    /// @brief Bind this to the parent `wxEvtHandler`'s close event
    ///     (or call from an existing close handler) to cancel
    ///     any download that still pending.
    void CancelPending() noexcept { m_cancelled = true; }

    /// @returns The read web file content from the last call to
    ///     Read(). This will be a @c char buffer that can be converted
    ///     into a @c wxString.
    const std::vector<char>& GetLastRead() const noexcept { return m_buffer; }

    /// @brief Attempts to connect to an URL and load its response.
    /// @param url The URL to check.
    /// @note This will not read or download the webpage, it will only get its response.\n
    ///     If using @c SetEventHandler() instead of @c SetAndBindEventHandler(),
    ///     this object's event handler will need to have its @c wxEVT_WEBREQUEST_DATA event
    ///     bound to object if calling this.
    /// @sa GetLastStatus(), GetLastUrl(), GetLastContentType().
    void RequestResponse(const wxString& url);

    /// @returns The last status from a read, download, or response request.
    [[nodiscard]]
    int GetLastStatus() const noexcept
        {
        return m_lastStatus;
        }

    /// @returns The last status message from a read, download, or response request.
    [[nodiscard]]
    const wxString& GetLastStatusText() const noexcept
        {
        return m_lastStatusText;
        }

    /// @returns The last status info from a read, download, or response request.
    /// @note This is debug info, usually the contents of a redirected error page.
    [[nodiscard]]
    const wxString& GetLastStatusInfo() const noexcept
        {
        return m_lastStatusInfo;
        }

    /// @returns The file name from the last read OneDrive document.
    [[nodiscard]]
    const wxString& GetLastOneDriveFileName() const noexcept
        {
        return m_lastOneDriveFileName;
        }

    /// @returns The last url (or possible redirect) from a read, download, or response request.
    [[nodiscard]]
    const wxString& GetLastUrl() const noexcept
        {
        return m_lastUrl;
        }

    /// @returns The path to the last downloaded file.\n
    ///     This is useful if using the suggested filename from the server, which will override
    ///     the path sent do Download().
    [[nodiscard]]
    const wxString& GetDownloadPath() const noexcept
        {
        return m_downloadPath;
        }

    /// @returns The last @c Content-Type from a read, download, or response request.
    [[nodiscard]]
    const wxString& GetLastContentType() const noexcept
        {
        return m_lastContentType;
        }

    /// @brief Bind this to a @c wxEVT_WEBREQUEST_STATE in the
    ///     parent @c wxEvtHandler.
    /// @param evt The event to process.
    /// @par Example:
    /// @code
    ///     // assuming m_downloadFile is a FileDownload member of the event handler
    ///     Bind(wxEVT_WEBREQUEST_STATE,
    ///          &FileDownload::ProcessRequest, &m_downloadFile);
    ///     Bind(wxEVT_WEBREQUEST_DATA,
    ///         &FileDownload::ProcessRequest, &m_downloadFile);
    /// @endcode
    void ProcessRequest(const wxWebRequestEvent& evt);

  private:
    void LoadResponseInfo(const wxWebRequestEvent& evt);

    /// @param restartTimer @true to restart the timer used to determine if
    ///     the response times out.
    void Reset(bool restartTimer)
        {
        m_lastStatusText.clear();
        m_downloadPath.clear();
        m_lastUrl.clear();
        m_lastSuggestedFileName.clear();
        m_buffer.clear();
        m_lastContentType.clear();
        m_lastStatusInfo.clear();
        m_server.clear();
        m_lastStatus = 404;
        m_downloadSuccessful = false;
        m_statusHasBeenProcessed = false;
        m_timedOut = false;
        m_downloadTooSmall = false;
        m_lastState = wxWebRequest::State::State_Idle;
        m_bytesReceived = 0;
        m_cancelled = false;
        if (restartTimer)
            {
            m_startTime = std::chrono::system_clock::now();
            }
        }

    wxEvtHandler* m_handler{ nullptr };
    mutable std::mutex m_mutex;
    std::vector<char> m_buffer;
    wxString m_userAgent;
    wxString m_cookies;
    std::optional<uint32_t> m_minFileDownloadSizeKilobytes{ std::nullopt };

    int m_timeoutSeconds{ 30 };
    int m_lastStatus{ 404 };
    bool m_disablePeerVerify{ false };
    bool m_useSuggestedFileName{ false };

    // state-based fields
    bool m_downloadSuccessful{ false };
    bool m_statusHasBeenProcessed{ false };
    bool m_timedOut{ false };
    bool m_downloadTooSmall{ false };
    bool m_cancelled{ false };
    wxString m_downloadPath;
    wxString m_lastStatusText;
    wxString m_lastUrl;
    wxString m_lastSuggestedFileName;
    wxString m_lastContentType;
    wxString m_lastStatusInfo;
    wxString m_lastOneDriveFileName;
    wxString m_server;
    wxFileOffset m_bytesReceived{ 0 };
    wxWebRequest::State m_lastState{ wxWebRequest::State::State_Idle };
    std::chrono::system_clock::time_point m_startTime;
    };

    /** @}*/

#endif // WISTERIA_DOWNLOADFILE_H
