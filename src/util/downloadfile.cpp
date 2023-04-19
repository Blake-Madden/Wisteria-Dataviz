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
    wxASSERT_MSG(m_handler,
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
        if (request.GetState() == wxWebRequest::State_Active ||
            request.GetState() == wxWebRequest::State_Unauthorized)
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
            if (!wxRenameFile(evt.GetDataFile(), downloadPath))
                wxLogError(L"Could not move %s", evt.GetDataFile());
            Remove(evt.GetId());
            }
        break;
        }
    case wxWebRequest::State_Failed:
        wxLogError(L"Web Request failed: %s", evt.GetErrorDescription());
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
        wxFALLTHROUGH;
    case wxWebRequest::State_Idle:
        break;
        }
    }

//--------------------------------------------------
bool FileDownload::Download(const wxString& url, const wxString& localDownloadPath)
    {
    wxASSERT_MSG(m_handler,
        L"Call SetEventHandler() to connect an event handler!");
    if (m_handler == nullptr)
        {
        wxLogError(L"Download could not start because event handler "
                    "has not been connected.");
        return false;
        }
    m_downloadPath = localDownloadPath;
    m_request = wxWebSession::GetDefault().CreateRequest(
        m_handler, url);
    m_request.SetStorage(wxWebRequest::Storage_File);
    m_stillActive = true;
    m_downloadSuccessful = false;
    m_request.Start();

    wxProgressDialog* progressDlg = m_showProgress ?
        new wxProgressDialog(wxTheApp->GetAppName(),
            _(L"Downloading "), 100, nullptr,
            wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_CAN_ABORT) :
        nullptr;

    while (m_stillActive)
        {
        wxYield();
        if (m_request.GetBytesExpectedToReceive() > 0)
            {
            if (progressDlg != nullptr &&
                !progressDlg->Update((m_request.GetBytesReceived() * 100) /
                m_request.GetBytesExpectedToReceive()))
                {
                m_request.Cancel();
                break;
                }
            }
        }
    if (progressDlg)
        { progressDlg->Close(); }
    return m_downloadSuccessful;
    }

//--------------------------------------------------
wxWebResponse FileDownload::GetResponse(const wxString& url)
    {
    wxASSERT_MSG(m_handler,
        L"Call SetEventHandler() to connect an event handler!");
    if (m_handler == nullptr)
        {
        wxLogError(L"Download could not start because event handler "
                    "has not been connected.");
        return wxWebResponse{};
        }
    m_downloadPath.clear();
    m_buffer.clear();
    m_request = wxWebSession::GetDefault().CreateRequest(
        m_handler, url);
    m_request.SetStorage(wxWebRequest::Storage_None);
    m_stillActive = true;
    m_request.Start();

    while (m_stillActive)
        { wxYield(); }
    return m_request.GetResponse();
    }

//--------------------------------------------------
bool FileDownload::Read(const wxString& url)
    {
    wxASSERT_MSG(m_handler,
        L"Call SetEventHandler() to connect an event handler!");
    if (m_handler == nullptr)
        {
        wxLogError(L"Download could not start because event handler "
                    "has not been connected.");
        return false;
        }
    m_downloadPath.clear();
    m_buffer.clear();
    m_request = wxWebSession::GetDefault().CreateRequest(
        m_handler, url);
    m_request.SetStorage(wxWebRequest::Storage_Memory);
    m_stillActive = true;
    m_request.Start();

    while (m_stillActive)
        { wxYield(); }
    return (m_request.GetState() == wxWebRequest::State_Completed);
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
            if (m_request.GetStorage() == wxWebRequest::Storage_File)
                {
                if (!wxRenameFile(evt.GetDataFile(), m_downloadPath))
                    { wxLogError(L"Could not move %s", evt.GetDataFile()); }
                else
                    { m_downloadSuccessful = true; }
                }
            // otherwise, it was requested to be read into a buffer
            else if (m_request.GetStorage() == wxWebRequest::Storage_Memory)
                {
                m_buffer.resize(evt.GetResponse().GetStream()->GetSize() + 1, 0);
                evt.GetResponse().GetStream()->ReadAll(&m_buffer[0], m_buffer.size() - 1);
                }
            m_stillActive = false;
            break;
            }
        case wxWebRequest::State_Failed:
            wxLogError(L"Web Request failed: %s", evt.GetErrorDescription());
            m_stillActive = false;
            break;
        case wxWebRequest::State_Cancelled:
            m_stillActive = false;
            break;
        case wxWebRequest::State_Unauthorized:
            {
            wxWebAuthChallenge
                auth = m_request.GetAuthChallenge();
            if (!auth.IsOk())
                {
                wxLogStatus(L"Unexpectedly missing auth challenge");
                m_stillActive = false;
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
            wxFALLTHROUGH;
        case wxWebRequest::State_Idle:
            m_stillActive = true;
            break;
        }
    }
