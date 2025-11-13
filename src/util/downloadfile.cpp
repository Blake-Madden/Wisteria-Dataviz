///////////////////////////////////////////////////////////////////////////////
// Name:        downloadfile.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "downloadfile.h"
#include "fileutil.h"
#include <chrono>

//--------------------------------------------------
void QueueDownload::Add(const wxString& url, const wxString& localDownloadPath)
    {
    assert(m_handler && L"Call SetEventHandler() to connect an event handler!");
    if (m_handler == nullptr)
        {
        wxLogError(L"Download queue could not start because event handler "
                   "has not been connected.");
        return;
        }

    wxWebRequest request = wxWebSession::GetDefault().CreateRequest(m_handler, url, m_currentId++);
    request.SetStorage(wxWebRequest::Storage_File);
    request.SetHeader(L"User-Agent", GetUserAgent());
    request.SetHeader(L"Sec-Fetch-Mode", _DT(L"navigate"));
    request.DisablePeerVerify(IsPeerVerifyDisabled());
    m_downloads.insert(std::make_pair(request.GetId(), localDownloadPath));
    m_requests.push_back(request);
    }

//--------------------------------------------------
wxString QueueDownload::GetLocalPath(const int ID) const
    {
    std::lock_guard<decltype(m_mutex)> lock{ m_mutex };
    auto downloadInfo = m_downloads.find(ID);
    return (downloadInfo != m_downloads.cend()) ? downloadInfo->second : wxString{};
    }

//--------------------------------------------------
void QueueDownload::Remove(const int ID)
    {
    std::lock_guard<decltype(m_mutex)> lock{ m_mutex };
    auto downloadInfo = m_downloads.find(ID);
    if (downloadInfo != m_downloads.cend())
        {
        m_downloads.erase(downloadInfo);
        }
    // Reset the ID if everything has been processed.
    if (m_downloads.empty())
        {
        m_currentId = 0;
        }
    }

//--------------------------------------------------
void QueueDownload::CancelPending()
    {
    std::ranges::for_each(m_requests,
                          [](wxWebRequest& request)
                          {
                              if (request.IsOk() &&
                                  (request.GetState() == wxWebRequest::State_Active ||
                                   request.GetState() == wxWebRequest::State_Unauthorized))
                                  {
                                  request.Cancel();
                                  }
                          });
    }

//--------------------------------------------------
void QueueDownload::ProcessRequest(wxWebRequestEvent& evt)
    {
    switch (evt.GetState())
        {
    // Request completed
    case wxWebRequest::State_Completed:
        {
        // get the stream's download path, based on the url's ID
        auto downloadPath = GetLocalPath(evt.GetId());
        if (!downloadPath.empty())
            {
            if (wxFileName::FileExists(downloadPath))
                {
                wxFileName(downloadPath).SetPermissions(wxS_DEFAULT);
                }

            if (!wxRenameFile(evt.GetDataFile(), downloadPath) &&
                !RenameFileShortenName(evt.GetDataFile(), downloadPath))
                {
                wxLogError(L"Could not move %s", evt.GetDataFile());
                }
            Remove(evt.GetId());
            }
        break;
        }
    case wxWebRequest::State_Failed:
        wxLogError(L"Web request failed: %s (%s)", evt.GetErrorDescription(),
                   QueueDownload::GetResponseMessage(evt.GetRequest().GetResponse().GetStatus()));
        Remove(evt.GetId());
        break;
    case wxWebRequest::State_Cancelled:
        Remove(evt.GetId());
        break;
    case wxWebRequest::State_Unauthorized:
        {
        const auto requestPos =
            std::ranges::find_if(std::as_const(m_requests), [&evt](const auto& request)
                                 { return request.GetId() == evt.GetId(); });
        if (requestPos == m_requests.cend())
            {
            Remove(evt.GetId());
            }
        else
            {
            if (!requestPos->GetAuthChallenge().IsOk())
                {
                wxLogStatus(L"Unexpectedly missing auth challenge");
                Remove(evt.GetId());
                break;
                }
            else if (IsPeerVerifyDisabled())
                {
                wxLogStatus(L"Credentials were requested, but will not be used because "
                            "SSL certificate verification is disabled.");
                Remove(evt.GetId());
                break;
                }

            wxWebCredentials cred;
            wxCredentialEntryDialog dialog(
                wxTheApp->GetTopWindow(),
                wxString::Format(_(L"Please enter credentials for accessing\n%s"),
                                 evt.GetResponse().GetURL()),
                wxTheApp->GetAppName(), cred);
            if (dialog.ShowModal() == wxID_OK)
                {
                requestPos->GetAuthChallenge().SetCredentials(cred);
                wxLogStatus(L"Trying to authenticate...");
                }
            else
                {
                wxLogStatus(L"Authentication challenge canceled");
                Remove(evt.GetId());
                }
            }
        break;
        }
    // Nothing special to do for these states.
    case wxWebRequest::State_Active:
        [[fallthrough]];
    case wxWebRequest::State_Idle:
        break;
        }
    }

//--------------------------------------------------
bool FileDownload::DownloadOneDriveFile(const wxString& url, const wxString& localDownloadFolder)
    {
    m_lastOneDriveFileName.clear();
    if (!Read(url))
        {
        return false;
        }
    const std::string_view oneDrivePage(GetLastRead().data(), GetLastRead().size());

    std::string fileName =
        lily_of_the_valley::html_extract_text::extract_onedrive_filename(oneDrivePage);
    std::string fileUrl =
        lily_of_the_valley::html_extract_text::extract_json_field(oneDrivePage, "FileUrlNoAuth");
    if (!Download(fileUrl, localDownloadFolder + wxFileName::GetPathSeparator() + fileName))
        {
        return false;
        }
    m_lastOneDriveFileName = fileName;
    return true;
    }

//--------------------------------------------------
bool FileDownload::Download(const wxString& url, const wxString& localDownloadPath)
    {
    wxASSERT_MSG(m_handler, L"Call SetEventHandler() to connect an event handler!");
    if (m_handler == nullptr)
        {
        wxLogError(L"Download could not start because event handler "
                   "has not been connected.");
        return false;
        }
    wxLogVerbose(L"Downloading '%s'", url);

    Reset(true);
    m_downloadPath = localDownloadPath;

    wxWebRequest request = wxWebSession::GetDefault().CreateRequest(m_handler, url);
    request.SetStorage(wxWebRequest::Storage_File);
    request.SetHeader(L"User-Agent", GetUserAgent());
    request.SetHeader(L"Sec-Fetch-Mode", _DT(L"navigate"));
    if (!GetCookies().empty())
        {
        request.SetHeader(_DT(L"Cookie", DTExplanation::InternalKeyword), GetCookies());
        }
    request.DisablePeerVerify(IsPeerVerifyDisabled());
    if (request.IsOk())
        {
        request.Start();

        while (!m_statusHasBeenProcessed)
            {
            wxYield();

            if (m_lastState == wxWebRequest::State_Active && !m_statusHasBeenProcessed)
                {
                if (m_timedOut || m_cancelled)
                    {
                    request.Cancel();
                    }
                else if (const auto elapsedSeconds = std::chrono::system_clock::now() - m_startTime;
                         std::chrono::duration_cast<std::chrono::seconds>(elapsedSeconds).count() >
                             GetTimeout() &&
                         m_bytesReceived == 0)
                    {
                    m_timedOut = true;
                    wxLogError(L"Page timed out after %s seconds. Response code #%d (%s).",
                               std::to_wstring(
                                   std::chrono::duration_cast<std::chrono::seconds>(elapsedSeconds)
                                       .count()),
                               m_lastStatus, QueueDownload::GetResponseMessage(m_lastStatus));
                    request.Cancel();
                    }
                }
            }

        if (m_timedOut)
            {
            // change status to "Page not responding" since we gave up after logging the real status
            m_lastStatus = 204;
            m_lastStatusText = _(L"Page not responding");
            m_downloadSuccessful = false;
            }
        else if (m_downloadTooSmall)
            {
            m_lastStatusText = _(L"File skipped; too small to download");
            m_downloadSuccessful = false;
            }

        return m_downloadSuccessful;
        }
    else
        {
        m_lastStatus = 204;
        m_lastStatusText = _(L"Unable to send request");
        return false;
        }
    }

//--------------------------------------------------
void FileDownload::RequestResponse(const wxString& url)
    {
    assert(m_handler && L"Call SetEventHandler() to connect an event handler!");
    if (m_handler == nullptr)
        {
        wxLogError(L"Download could not start because event handler "
                   "has not been connected.");
        return;
        }
    // note that you need to printf the string before passing to wxLog
    // because this is an untrusted string (i.e., and URL that can contain '%' in it).
    wxLogVerbose(L"Requesting response from '%s'", url);

    Reset(true);

    wxWebRequest request = wxWebSession::GetDefault().CreateRequest(m_handler, url);
    request.SetStorage(wxWebRequest::Storage_None);
    request.SetHeader(L"User-Agent", GetUserAgent());
    request.SetHeader(L"Sec-Fetch-Mode", _DT(L"navigate"));
    if (!GetCookies().empty())
        {
        request.SetHeader(L"Cookie", GetCookies());
        }
    request.DisablePeerVerify(IsPeerVerifyDisabled());
    if (request.IsOk())
        {
        request.Start();

        while (!m_statusHasBeenProcessed)
            {
            wxYield();

            if (m_lastState == wxWebRequest::State_Active && !m_statusHasBeenProcessed)
                {
                if (m_timedOut || m_cancelled)
                    {
                    request.Cancel();
                    }
                // also check time out this way in case we are stuck in Idle and the event
                // is no longer being processed
                else if (const auto elapsedSeconds = std::chrono::system_clock::now() - m_startTime;
                         std::chrono::duration_cast<std::chrono::seconds>(elapsedSeconds).count() >
                             GetTimeout() &&
                         m_bytesReceived == 0)
                    {
                    m_timedOut = true;
                    wxLogError(L"Page timed out after %s seconds. Response code #%d (%s).",
                               std::to_wstring(
                                   std::chrono::duration_cast<std::chrono::seconds>(elapsedSeconds)
                                       .count()),
                               m_lastStatus, QueueDownload::GetResponseMessage(m_lastStatus));
                    request.Cancel();
                    }
                }
            }
        wxLogVerbose(L"Requesting response from '%s' complete.", url);

        if (m_timedOut)
            {
            // change status to "Page not responding" since we gave up after logging the real status
            m_lastStatus = 204;
            m_lastStatusText = _(L"Page not responding");
            }
        }
    else
        {
        m_lastStatus = 204;
        m_lastStatusText = _(L"Unable to send request");
        }
    }

//--------------------------------------------------
bool FileDownload::ReadOneDriveFile(const wxString& url)
    {
    m_lastOneDriveFileName.clear();
    if (!Read(url))
        {
        return false;
        }
    const std::string_view oneDrivePage(GetLastRead().data(), GetLastRead().size());

    std::string fileName =
        lily_of_the_valley::html_extract_text::extract_onedrive_filename(oneDrivePage);
    std::string fileUrl =
        lily_of_the_valley::html_extract_text::extract_json_field(oneDrivePage, "FileUrlNoAuth");
    if (!Read(fileUrl))
        {
        return false;
        }
    m_lastOneDriveFileName = fileName;
    return true;
    }

//--------------------------------------------------
bool FileDownload::Read(const wxString& url)
    {
    wxASSERT_MSG(m_handler, L"Call SetEventHandler() to connect an event handler!");
    if (m_handler == nullptr)
        {
        wxLogError(L"Download could not start because event handler "
                   "has not been connected.");
        return false;
        }
    wxLogVerbose(L"Reading '%s'", url);

    Reset(true);

    wxWebRequest request = wxWebSession::GetDefault().CreateRequest(m_handler, url);
    request.SetStorage(wxWebRequest::Storage_Memory);
    request.SetHeader(L"User-Agent", GetUserAgent());
    request.SetHeader(L"Sec-Fetch-Mode", _DT(L"navigate"));
    if (!GetCookies().empty())
        {
        request.SetHeader(L"Cookie", GetCookies());
        }
    request.DisablePeerVerify(IsPeerVerifyDisabled());
    if (request.IsOk())
        {
        request.Start();

        while (!m_statusHasBeenProcessed)
            {
            wxYield();

            if (m_lastState == wxWebRequest::State_Active && !m_statusHasBeenProcessed)
                {
                if (m_timedOut || m_cancelled)
                    {
                    request.Cancel();
                    }
                else if (const auto elapsedSeconds = std::chrono::system_clock::now() - m_startTime;
                         std::chrono::duration_cast<std::chrono::seconds>(elapsedSeconds).count() >
                             GetTimeout() &&
                         m_bytesReceived == 0)
                    {
                    m_timedOut = true;
                    wxLogError(L"Page timed out after %s seconds. Response code #%d (%s).",
                               std::to_wstring(
                                   std::chrono::duration_cast<std::chrono::seconds>(elapsedSeconds)
                                       .count()),
                               m_lastStatus, QueueDownload::GetResponseMessage(m_lastStatus));
                    request.Cancel();
                    }
                }
            }

        if (m_timedOut)
            {
            // change status to "Page not responding" since we gave up after logging the real status
            m_lastStatus = 204;
            m_lastStatusText = _(L"Page not responding");
            }

        return (m_lastState == wxWebRequest::State_Completed);
        }
    else
        {
        m_lastStatus = 204;
        m_lastStatusText = _(L"Unable to send request");
        return false;
        }
    }

//--------------------------------------------------
void FileDownload::LoadResponseInfo(const wxWebRequestEvent& evt)
    {
    wxLogVerbose(L"Processing response info...");
    m_server = ((evt.GetRequest().IsOk() && evt.GetResponse().IsOk()) ?
                    evt.GetResponse().GetHeader(_DT(L"Server")) :
                    wxString{});
    m_lastStatus =
        ((evt.GetRequest().IsOk() && evt.GetResponse().IsOk()) ? evt.GetResponse().GetStatus() :
                                                                 404);
    m_lastStatusText =
        ((evt.GetRequest().IsOk() && evt.GetResponse().IsOk()) ? evt.GetResponse().GetStatusText() :
                                                                 wxString{});
    m_lastUrl =
        ((evt.GetRequest().IsOk() && evt.GetResponse().IsOk()) ? evt.GetResponse().GetURL() :
                                                                 wxString{});
    m_lastSuggestedFileName = ((evt.GetRequest().IsOk() && evt.GetResponse().IsOk()) ?
                                   evt.GetResponse().GetSuggestedFileName() :
                                   wxString{});
    m_lastContentType = ((evt.GetRequest().IsOk() && evt.GetResponse().IsOk()) ?
                             evt.GetResponse().GetHeader(_DT(L"Content-Type")) :
                             wxString{});
    m_lastStatusInfo =
        ((evt.GetRequest().IsOk() && evt.GetResponse().IsOk()) ? evt.GetResponse().AsString() :
                                                                 wxString{});
    m_lastState =
        (evt.GetRequest().IsOk() ? evt.GetRequest().GetState() : wxWebRequest::State::State_Failed);
    // if a redirected error page, parse it down to its readable content
    if (m_lastStatus != 200)
        {
        wxLogVerbose(L"Processing response status info...");
        lily_of_the_valley::html_extract_text hExtract;
        hExtract.include_no_script_sections(true);
        const wchar_t* const filteredMsg =
            hExtract(m_lastStatusInfo.wc_str(), m_lastStatusInfo.length(), true, false);
        if (filteredMsg != nullptr && hExtract.get_filtered_text_length())
            {
            m_lastStatusInfo.assign(filteredMsg, hExtract.get_filtered_text_length());
            m_lastStatusInfo.Trim(true).Trim(false);
            wxLogVerbose(L"Full response: %s", m_lastStatusInfo);
            }
        else
            {
            wxLogVerbose(L"Full response: %s", m_lastStatusInfo);
            }
        // Cloudflare forces the use of javascript to block robots
        if (m_lastStatus == 403 && m_server.CmpNoCase(_DT(L"cloudflare")) == 0)
            {
            m_lastStatusInfo.insert(
                0, _(L"Webpage is using Cloudflare protection and "
                     "can only be accessed via an interactive browser. "
                     "Please use a browser to download this page.\n\nResponse from website:\n"));
            }
        }
    }

//--------------------------------------------------
void FileDownload::ProcessRequest(const wxWebRequestEvent& evt)
    {
    if (evt.GetRequest().IsOk())
        {
        m_bytesReceived = evt.GetRequest().GetBytesReceived();
        m_lastState = evt.GetRequest().GetState();
        if (evt.GetRequest().GetResponse().IsOk())
            {
            m_lastSuggestedFileName = evt.GetRequest().GetResponse().GetSuggestedFileName();
            }
        }

    switch (evt.GetState())
        {
    // Request completed
    case wxWebRequest::State_Completed:
        {
        // if file was downloaded to a temp file,
        // copy it to the requested location
        if (evt.GetRequest().GetStorage() == wxWebRequest::Storage_File)
            {
            if (m_useSuggestedFileName && !m_lastSuggestedFileName.empty())
                {
                wxFileName fn{ m_downloadPath };
                wxString originalExt{ fn.GetExt() };
                fn.SetFullName(m_lastSuggestedFileName);
                if (!fn.HasExt())
                    {
                    fn.SetExt(originalExt);
                    }
                m_downloadPath = fn.GetFullPath();
                }

            if (wxFileName::FileExists(m_downloadPath))
                {
                wxFileName(m_downloadPath).SetPermissions(wxS_DEFAULT);
                }

            /* Check size constraints (if in use)
               to see if we should "download" it to the final destination.

               Note that we need to check file size constraints here after already
               downloading because GetBytesExpectedToReceive() during the connection
               stage can be packet size and not reflect the ultimate file size.*/
            if (wxFileName::FileExists(evt.GetDataFile()) &&
                m_minFileDownloadSizeKilobytes.has_value() &&
                m_minFileDownloadSizeKilobytes.value() >
                    (wxFileName::GetSize(evt.GetDataFile()) / 1024))
                {
                m_downloadTooSmall = true;
                }
            else if (!wxRenameFile(evt.GetDataFile(), m_downloadPath) &&
                     !RenameFileShortenName(evt.GetDataFile(), m_downloadPath))
                {
                wxLogError(L"Could not move %s", evt.GetDataFile());
                }
            else
                {
                m_downloadSuccessful = true;
                }
            }
        // otherwise, it was requested to be read into a buffer
        else if (evt.GetRequest().GetStorage() == wxWebRequest::Storage_Memory)
            {
            m_buffer.resize(evt.GetResponse().GetStream()->GetSize() + 1, 0);
            if (m_buffer.size() > 1)
                {
                evt.GetResponse().GetStream()->ReadAll(&m_buffer[0], m_buffer.size() - 1);
                }
            }
        m_statusHasBeenProcessed = true;
        LoadResponseInfo(evt);
        break;
        }
    case wxWebRequest::State_Failed:
        if (evt.GetRequest().IsOk() && evt.GetRequest().GetResponse().IsOk())
            {
            wxLogError(
                L"'%s', web request failed: %s (%s)", evt.GetRequest().GetResponse().GetURL(),
                evt.GetErrorDescription(),
                QueueDownload::GetResponseMessage(evt.GetRequest().GetResponse().GetStatus()));
            }
        else
            {
            wxLogError(L"Web request failed: %s", evt.GetErrorDescription());
            }
        m_statusHasBeenProcessed = true;
        LoadResponseInfo(evt);
        break;
    case wxWebRequest::State_Cancelled:
        m_statusHasBeenProcessed = true;
        LoadResponseInfo(evt);
        break;
    case wxWebRequest::State_Unauthorized:
        {
        if (evt.GetRequest().IsOk() && !evt.GetRequest().GetAuthChallenge().IsOk())
            {
            wxLogStatus(L"Unexpectedly missing authentication challenge");
            break;
            }
        if (IsPeerVerifyDisabled())
            {
            wxLogStatus(L"Credentials were requested, but will not be used because "
                        "SSL certificate verification is disabled.");
            break;
            }

        const wxWebCredentials cred;
        wxCredentialEntryDialog dialog(
            wxTheApp->GetTopWindow(),
            wxString::Format(_(L"Please enter credentials for accessing\n%s"),
                             evt.GetResponse().GetURL()),
            wxTheApp->GetAppName(), cred);
        if (dialog.ShowModal() == wxID_OK)
            {
            evt.GetRequest().GetAuthChallenge().SetCredentials(cred);
            wxLogStatus(L"Trying to authenticate...");
            }
        else
            {
            wxLogStatus(L"Authentication challenge canceled");
            }
        m_statusHasBeenProcessed = true;
        LoadResponseInfo(evt);
        break;
        }
    case wxWebRequest::State_Active:
        [[fallthrough]];
    case wxWebRequest::State_Idle:
        /* Check after XX seconds whether any data has been received;
           if not, then quit.*/
        if (const auto elapsedSeconds = std::chrono::system_clock::now() - m_startTime;
            std::chrono::duration_cast<std::chrono::seconds>(elapsedSeconds).count() >
                GetTimeout() &&
            m_bytesReceived == 0)
            {
            wxLogError(
                L"Page timed out after %s seconds. Response code #%d (%s).",
                std::to_wstring(
                    std::chrono::duration_cast<std::chrono::seconds>(elapsedSeconds).count()),
                m_lastStatus, QueueDownload::GetResponseMessage(m_lastStatus));
            LoadResponseInfo(evt);
            m_timedOut = true;
            }
        break;
        }
    }
