///////////////////////////////////////////////////////////////////////////////
// Name:        downloadfile.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include "downloadfile.h"
#include "fileutil.h"

//--------------------------------------------------
void QueueDownload::Add(const wxString& url, const wxString& localDownloadPath)
    {
    assert(m_handler &&
           L"Call SetEventHandler() to connect an event handler!");
    if (m_handler == nullptr)
        {
        wxLogError(L"Download queue could not start because event handler "
                    "has not been connected.");
        return;
        }

    wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
        m_handler, url, m_currentId++);
    request.SetStorage(wxWebRequest::Storage_File);
    request.SetHeader(L"User-Agent", GetUserAgent());
    request.SetHeader(L"Sec-Fetch-Mode", L"navigate");
    request.DisablePeerVerify(IsPeerVerifyDisabled());
    m_downloads.insert(std::make_pair(request.GetId(), localDownloadPath));
    m_requests.push_back(request);
    }

//--------------------------------------------------
wxString QueueDownload::GetLocalPath(const int ID) const
    {
    std::lock_guard<decltype(m_mutex)> lock{ m_mutex };
    auto downloadInfo = m_downloads.find(ID);
    return (downloadInfo != m_downloads.cend()) ?
        downloadInfo->second : wxString{};
    }

//--------------------------------------------------
void QueueDownload::Remove(const int ID)
    {
    std::lock_guard<decltype(m_mutex)> lock{ m_mutex };
    auto downloadInfo = m_downloads.find(ID);
    if (downloadInfo != m_downloads.cend())
        { m_downloads.erase(downloadInfo); }
    // Reset the ID if everything has been processed.
    if (m_downloads.size() == 0)
        { m_currentId = 0; }
    }

//--------------------------------------------------
void QueueDownload::CancelPending()
    {
    std::for_each(m_requests.begin(), m_requests.end(),
        [](wxWebRequest& request)
        {
        if (request.IsOk() &&
            (request.GetState() == wxWebRequest::State_Active ||
             request.GetState() == wxWebRequest::State_Unauthorized))
            { request.Cancel(); }
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
                { wxFileName(downloadPath).SetPermissions(wxS_DEFAULT); }

            if (!wxRenameFile(evt.GetDataFile(), downloadPath) &&
                !RenameFileShortenName(evt.GetDataFile(), downloadPath) )
                { wxLogError(L"Could not move %s", evt.GetDataFile()); }
            Remove(evt.GetId());
            }
        break;
        }
    case wxWebRequest::State_Failed:
        wxLogError(L"Web Request failed: %s (%s)",
            evt.GetErrorDescription(),
            QueueDownload::GetResponseMessage(evt.GetRequest().GetResponse().GetStatus()));
        Remove(evt.GetId());
        break;
    case wxWebRequest::State_Cancelled:
        Remove(evt.GetId());
        break;
    case wxWebRequest::State_Unauthorized:
        {
        const auto requestPos = std::find_if(m_requests.cbegin(), m_requests.cend(),
            [&evt](const auto& request)
            {
                return request.GetId() == evt.GetId();
            });
        if (requestPos == m_requests.cend())
            { Remove(evt.GetId()); }
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
            wxCredentialEntryDialog dialog
                (
                wxTheApp->GetTopWindow(),
                wxString::Format
                    (_(L"Please enter credentials for accessing\n%s"),
                    evt.GetResponse().GetURL() ),
                wxTheApp->GetAppName(),
                cred
                );
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
bool FileDownload::Download(const wxString& url, const wxString& localDownloadPath)
    {
    assert(m_handler &&
        L"Call SetEventHandler() to connect an event handler!");
    if (m_handler == nullptr)
        {
        wxLogError(L"Download could not start because event handler "
                    "has not been connected.");
        return false;
        }
    wxLogVerbose(L"Downloading '%s'", url);
    m_downloadPath = localDownloadPath;
    wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
        m_handler, url);
    request.SetStorage(wxWebRequest::Storage_File);
    request.SetHeader(L"User-Agent", GetUserAgent());
    request.SetHeader(L"Sec-Fetch-Mode", L"navigate");
    request.DisablePeerVerify(IsPeerVerifyDisabled());
    m_lastStatus = 404;
    m_downloadSuccessful = false;
    request.Start();

    wxProgressDialog* progressDlg = m_showProgress ?
        new wxProgressDialog(wxTheApp->GetAppName(),
            _(L"Downloading "), 100, nullptr,
            wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_CAN_ABORT) :
        nullptr;

    const auto startTime = std::chrono::system_clock::now();
    bool timedOut{ false };
    while (request.GetState() == wxWebRequest::State_Active)
        {
        wxYield();
        if (progressDlg != nullptr &&
            request.GetBytesExpectedToReceive() > 0)
            {
            if (!progressDlg->Update((request.GetBytesReceived() * 100) /
                                      request.GetBytesExpectedToReceive()))
                {
                request.Cancel();
                break;
                }
            }
        const auto rightNow = std::chrono::system_clock::now();
        const auto elapsedSeconds = rightNow - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsedSeconds).count() >
            GetTimeout() &&
            request.GetBytesReceived() == 0)
            {
            m_lastStatus = ((request.IsOk() && request.GetResponse().IsOk()) ?
                     request.GetResponse().GetStatus() : 404);
            wxLogError(L"Downloading page timed out after %s seconds. Response code #%d.",
                std::to_wstring(std::chrono::duration_cast<std::chrono::seconds>
                    (elapsedSeconds).count()), m_lastStatus);
            timedOut = true;
            request.Cancel();
            }
        }
    if (progressDlg != nullptr)
        { progressDlg->Close(); }

    LoadResponseInfo(request);
    if (timedOut)
        {
        // change status to "Page not responding" since we gave up after logging the real status
        m_lastStatus = 204;
        m_lastStatusText = _(L"Page not responding");
        m_downloadSuccessful = false;
        }

    return m_downloadSuccessful;
    }

//--------------------------------------------------
void FileDownload::RequestResponse(const wxString& url)
    {
    assert(m_handler &&
        L"Call SetEventHandler() to connect an event handler!");
    if (m_handler == nullptr)
        {
        wxLogError(L"Download could not start because event handler "
                    "has not been connected.");
        return;
        }
    // note that you need to printf the string before passing to wxLog
    // because this is an untrusted string (i.e., and URL that can contain '%' in it).
    wxLogVerbose(L"Requesting response from '%s'", url);
    m_downloadPath.clear();
    m_buffer.clear();
    wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
        m_handler, url);
    request.SetStorage(wxWebRequest::Storage_None);
    request.SetHeader(L"User-Agent", GetUserAgent());
    request.SetHeader(L"Sec-Fetch-Mode", L"navigate");
    request.DisablePeerVerify(IsPeerVerifyDisabled());
    m_lastStatus = 404;
    request.Start();

    const auto startTime = std::chrono::system_clock::now();
    bool timedOut{ false };
    while (request.GetState() == wxWebRequest::State_Active)
        {
        wxYield();
        /* Some misconfigured webpages cause ProcessRequest() to not be called
           after some sort of failure , meaning that we won't have a change to
           query its state and truly see that it is no longer active.
           Time out after XX seconds since requesting a response should only involve
           pinging the server and shouldn't take very long.*/
        const auto rightNow = std::chrono::system_clock::now();
        const auto elapsedSeconds = rightNow - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsedSeconds).count() >
            GetTimeout())
            {
            m_lastStatus = ((request.IsOk() && request.GetResponse().IsOk()) ?
                     request.GetResponse().GetStatus() : 404);
            wxLogError(L"Requesting response timed out after %s seconds. Response code #%d.",
                std::to_wstring(std::chrono::duration_cast<std::chrono::seconds>
                    (elapsedSeconds).count()), m_lastStatus);
            timedOut = true;
            request.Cancel();
            }
        }
    wxLogVerbose(L"Requesting response from '%s' complete.", url);

    LoadResponseInfo(request);
    if (timedOut)
        {
        // change status to "Page not responding" since we gave up after logging the real status
        m_lastStatus = 204;
        m_lastStatusText = _(L"Page not responding");
        }
    }

//--------------------------------------------------
bool FileDownload::Read(const wxString& url)
    {
    assert(m_handler &&
        L"Call SetEventHandler() to connect an event handler!");
    if (m_handler == nullptr)
        {
        wxLogError(L"Download could not start because event handler "
                    "has not been connected.");
        return false;
        }
    wxLogVerbose(L"Reading '%s'", url);
    m_downloadPath.clear();
    m_buffer.clear();

    wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
        m_handler, url);
    request.SetStorage(wxWebRequest::Storage_Memory);
    request.SetHeader(L"User-Agent", GetUserAgent());
    request.SetHeader(L"Sec-Fetch-Mode", L"navigate");
    request.DisablePeerVerify(IsPeerVerifyDisabled());
    m_lastStatus = 404;
    request.Start();

    const auto startTime = std::chrono::system_clock::now();
    bool timedOut{ false };
    while (request.GetState() == wxWebRequest::State_Active)
        {
        wxYield();
        /* Sometimes a connection failure will cause ProcessRequest to not be called,
           meaning that the active flag won't be turned as expected. Check after
           XX seconds as to whether any data has been received; if not, then quit.*/
        const auto rightNow = std::chrono::system_clock::now();
        const auto elapsedSeconds = rightNow - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsedSeconds).count() >
            GetTimeout() &&
            request.GetBytesReceived() == 0)
            {
            m_lastStatus = ((request.IsOk() && request.GetResponse().IsOk()) ?
                     request.GetResponse().GetStatus() : 404);
            wxLogError(L"Reading page timed out after %s seconds. Response code #%d.",
                std::to_wstring(std::chrono::duration_cast<std::chrono::seconds>
                    (elapsedSeconds).count()), m_lastStatus);
            timedOut = true;
            request.Cancel();
            }
        }

    LoadResponseInfo(request);
    if (timedOut)
        {
        // change status to "Page not responding" since we gave up after logging the real status
        m_lastStatus = 204;
        m_lastStatusText = _(L"Page not responding");
        }

    return (request.GetState() == wxWebRequest::State_Completed);
    }

//--------------------------------------------------
void FileDownload::LoadResponseInfo(const wxWebRequest& request)
    {
    m_server = ((request.IsOk() && request.GetResponse().IsOk()) ?
            request.GetResponse().GetHeader("Server") : wxString{});
    m_lastStatus = ((request.IsOk() && request.GetResponse().IsOk()) ?
                     request.GetResponse().GetStatus() : 404);
    m_lastStatusText = ((request.IsOk() && request.GetResponse().IsOk()) ?
                    request.GetResponse().GetStatusText() : wxString{});
    m_lastUrl = ((request.IsOk() && request.GetResponse().IsOk()) ?
                  request.GetResponse().GetURL() : wxString{});
    m_lastContentType = ((request.IsOk() && request.GetResponse().IsOk()) ?
                          request.GetResponse().GetHeader("Content-Type") : wxString{});
    m_lastStatusInfo = ((request.IsOk() && request.GetResponse().IsOk()) ?
                    request.GetResponse().AsString() : wxString{});
    // if a redirected error page, parse it down to its readable content
    if (m_lastStatus != 200)
        {
        lily_of_the_valley::html_extract_text hExtract;
        hExtract.include_no_script_sections(true);
        const wchar_t* const filteredMsg =
            hExtract(m_lastStatusInfo.wc_str(), m_lastStatusInfo.length(), true, false);
        if (filteredMsg != nullptr && hExtract.get_filtered_text_length())
            {
            m_lastStatusInfo.assign(filteredMsg);
            m_lastStatusInfo.Trim(true).Trim(false);
            }
        // Cloudflare forces the use of javascript to block robots
        if (m_lastStatus == 403 && m_server.CmpNoCase(L"cloudflare") == 0)
            {
            m_lastStatusInfo.insert(0, _(L"Webpage is using Cloudflare protection and "
                "can only be accessed via an interactive browser. "
                "Please use a browser to download this page.\n\nResponse from website:\n"));
            }
        }
    }

//--------------------------------------------------
void FileDownload::ProcessRequest(wxWebRequestEvent& evt)
    {
    switch (evt.GetState())
        {
        // Request completed
        case wxWebRequest::State_Completed:
            {
            // if file was downloaded to a temp file,
            // copy it to the requested location
            if (evt.GetRequest().GetStorage() == wxWebRequest::Storage_File)
                {
                if (wxFileName::FileExists(m_downloadPath))
                    { wxFileName(m_downloadPath).SetPermissions(wxS_DEFAULT); }

                if (!wxRenameFile(evt.GetDataFile(), m_downloadPath) &&
                    !RenameFileShortenName(evt.GetDataFile(), m_downloadPath) )
                    { wxLogError(L"Could not move %s", evt.GetDataFile()); }
                else
                    { m_downloadSuccessful = true; }
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
            break;
            }
        case wxWebRequest::State_Failed:
            if (evt.GetRequest().IsOk() && evt.GetRequest().GetResponse().IsOk())
                {
                wxLogError(L"Web Request failed: %s (%s)",
                    evt.GetErrorDescription(),
                    QueueDownload::GetResponseMessage(evt.GetRequest().GetResponse().GetStatus()));
                }
            else
                {
                wxLogError(L"Web Request failed: %s",
                    evt.GetErrorDescription());
                }
            break;
        case wxWebRequest::State_Cancelled:
            break;
        case wxWebRequest::State_Unauthorized:
            {
            if (evt.GetRequest().IsOk() &&
                !evt.GetRequest().GetAuthChallenge().IsOk())
                {
                wxLogStatus(L"Unexpectedly missing authentication challenge");
                break;
                }
            else if (IsPeerVerifyDisabled())
                {
                wxLogStatus(L"Credentials were requested, but will not be used because "
                             "SSL certificate verification is disabled.");
                break;
                }

            wxWebCredentials cred;
            wxCredentialEntryDialog dialog(
                wxTheApp->GetTopWindow(),
                wxString::Format(
                    _(L"Please enter credentials for accessing\n%s"),
                    evt.GetResponse().GetURL() ), wxTheApp->GetAppName(), cred);
            if (dialog.ShowModal() == wxID_OK)
                {
                evt.GetRequest().GetAuthChallenge().SetCredentials(cred);
                wxLogStatus(L"Trying to authenticate...");
                }
            else
                {
                wxLogStatus(L"Authentication challenge canceled");
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
