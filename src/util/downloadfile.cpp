///////////////////////////////////////////////////////////////////////////////
// Name:        downloadfile.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "downloadfile.h"

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

            if (!wxRenameFile(evt.GetDataFile(), downloadPath))
                { wxLogError(L"Could not move %s", evt.GetDataFile()); }
            Remove(evt.GetId());
            }
        break;
        }
    case wxWebRequest::State_Failed:
        wxLogError(L"Web Request failed: %s (%s)",
                   evt.GetErrorDescription(), QueueDownload::GetResponseMessage(evt.GetRequest().GetResponse().GetStatus()));
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
            wxWebAuthChallenge
                auth = requestPos->GetAuthChallenge();
            if (!auth.IsOk())
                {
                wxLogStatus(L"Unexpectedly missing auth challenge");
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
                auth.SetCredentials(cred);
                wxLogStatus(L"Trying to authenticate...");
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
    m_downloadPath = localDownloadPath;
    wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
        m_handler, url);
    request.SetStorage(wxWebRequest::Storage_File);
    request.SetHeader(L"User-Agent", GetUserAgent());
    request.SetHeader(L"Sec-Fetch-Mode", L"navigate");
    m_lastStatus = 404;
    m_lastState = wxWebRequest::State_Failed;
    m_stillActive = true;
    m_downloadSuccessful = false;
    wxWebRequest tempRequest{ request };
    request.Start();

    wxProgressDialog* progressDlg = m_showProgress ?
        new wxProgressDialog(wxTheApp->GetAppName(),
            _(L"Downloading "), 100, nullptr,
            wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_CAN_ABORT) :
        nullptr;

    while (m_stillActive)
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
        }
    if (progressDlg)
        { progressDlg->Close(); }
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
    m_downloadPath.clear();
    m_buffer.clear();
    wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
        m_handler, url);
    request.SetStorage(wxWebRequest::Storage_None);
    request.SetHeader(L"User-Agent", GetUserAgent());
    request.SetHeader(L"Sec-Fetch-Mode", L"navigate");
    m_lastStatus = 404;
    m_lastState = wxWebRequest::State_Failed;
    m_stillActive = true;
    request.Start();

    while (m_stillActive)
        { wxYield(); }
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
    m_downloadPath.clear();
    m_buffer.clear();

    wxWebRequest request = wxWebSession::GetDefault().CreateRequest(
        m_handler, url);
    request.SetStorage(wxWebRequest::Storage_Memory);
    request.SetHeader(L"User-Agent", GetUserAgent());
    request.SetHeader(L"Sec-Fetch-Mode", L"navigate");
    m_lastStatus = 404;
    m_lastState = wxWebRequest::State_Failed;
    m_stillActive = true;
    request.Start();

    while (m_stillActive)
        { wxYield(); }

    return (m_lastState == wxWebRequest::State_Completed);
    }

//--------------------------------------------------
void FileDownload::ProcessRequest(wxWebRequestEvent& evt)
    {
    // wxWebResponse does not get deep copied, so cached the response information that we
    // want for later here.
    const auto fillResponseInfo = [this, &evt]()
        {
        m_server = ((evt.GetRequest().IsOk() && evt.GetRequest().GetResponse().IsOk()) ?
            evt.GetRequest().GetResponse().GetHeader("Server") : wxString{});
        m_lastStatus = ((evt.GetRequest().IsOk() && evt.GetRequest().GetResponse().IsOk()) ?
                         evt.GetRequest().GetResponse().GetStatus() : 404);
        m_lastStatusText = ((evt.GetRequest().IsOk() && evt.GetRequest().GetResponse().IsOk()) ?
                        evt.GetRequest().GetResponse().GetStatusText() : wxString{});
        m_lastState = (evt.GetRequest().IsOk() ? evt.GetRequest().GetState() : wxWebRequest::State_Failed);
        m_lastUrl = ((evt.GetRequest().IsOk() && evt.GetRequest().GetResponse().IsOk()) ?
                      evt.GetRequest().GetResponse().GetURL() : wxString{});
        m_lastContentType = ((evt.GetRequest().IsOk() && evt.GetRequest().GetResponse().IsOk()) ?
                              evt.GetRequest().GetResponse().GetHeader("Content-Type") : wxString{});
        m_lastStatusInfo = ((evt.GetRequest().IsOk() && evt.GetRequest().GetResponse().IsOk()) ?
                      evt.GetRequest().GetResponse().AsString() : wxString{});
        // if a redirected error page, parse it down to its readable content
        if (m_lastStatus != 200)
            {
            lily_of_the_valley::html_extract_text hExtract;
            hExtract.include_no_script_sections(true);
            auto filteredMsg = hExtract(m_lastStatusInfo.wc_str(), m_lastStatusInfo.length(), true, false);
            if (filteredMsg && hExtract.get_filtered_text_length())
                {
                m_lastStatusInfo.assign(filteredMsg);
                m_lastStatusInfo.Trim(true).Trim(false);
                }
            // Cloudflare forces the use of javascript to block robots
            if (m_lastStatus == 403 && m_server.CmpNoCase(L"cloudflare") == 0)
                {
                m_lastStatusInfo.insert(0, _(L"Webpage is using Cloudflare protection and "
                    "can only be accessed via an interactive browser. Please use a browser to download "
                    "this page.\n\nResponse from website:\n"));
                }
            }
        };

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

                if (!wxRenameFile(evt.GetDataFile(), m_downloadPath))
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
            m_stillActive = false;
            fillResponseInfo();
            break;
            }
        case wxWebRequest::State_Failed:
            wxLogError(L"Web Request failed: %s (%s)",
                       evt.GetErrorDescription(), QueueDownload::GetResponseMessage(evt.GetRequest().GetResponse().GetStatus()));
            m_stillActive = false;
            fillResponseInfo();
            break;
        case wxWebRequest::State_Cancelled:
            m_stillActive = false;
            fillResponseInfo();
            break;
        case wxWebRequest::State_Unauthorized:
            {
            wxWebAuthChallenge
                auth = evt.GetRequest().GetAuthChallenge();
            if (!auth.IsOk())
                {
                wxLogStatus(L"Unexpectedly missing authentication challenge");
                m_stillActive = false;
                fillResponseInfo();
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
                auth.SetCredentials(cred);
                wxLogStatus(L"Trying to authenticate...");
                }
            break;
            }
        // Nothing special to do for these states.
        case wxWebRequest::State_Active:
            [[fallthrough]];
        case wxWebRequest::State_Idle:
            m_stillActive = true;
            break;
        }
    }
