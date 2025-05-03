///////////////////////////////////////////////////////////////////////////////
// Name:        logfile.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "logfile.h"

//--------------------------------------------------
LogFile::LogFile(bool clearPreviousLog)
    : // will be a unique file name on a per day basis
      m_logFilePath(wxStandardPaths::Get().GetTempDir() + wxFileName::GetPathSeparator() +
                    wxTheApp->GetAppName() + wxGetUserName() + wxDateTime::Now().FormatISODate() +
                    L".log")
    {
    wxFile logFile(m_logFilePath, clearPreviousLog ? wxFile::write : wxFile::write_append);
    if (!logFile.IsOpened())
        {
        wxMessageBox(wxString::Format(_(L"Unable to create log file at '%s'"), m_logFilePath),
                     _(L"Logging Error"), wxOK | wxICON_WARNING);
        return;
        }
    // clear file (from a previous program run) or prepare for appending
    logFile.Write(wxString{});
    }

//--------------------------------------------------
wxString LogFile::Read()
    {
    LogFile::Flush();

    wxString logBuffer;
    wxFile logFile(m_logFilePath, wxFile::read);
    if (logFile.IsOpened())
        {
        logFile.ReadAll(&logBuffer);
        }
    // flushing to temp file failed somehow,
    // so return whatever is queued up at least
    else
        {
        return m_buffer;
        }
    return logBuffer;
    }

//--------------------------------------------------
bool LogFile::Clear()
    {
    m_buffer.clear();

    wxFile logFile(m_logFilePath, wxFile::write);
    if (logFile.IsOpened())
        {
        return logFile.Write(wxString{});
        }
    return false;
    }

//--------------------------------------------------
void LogFile::Flush()
    {
    wxLog::Flush();
    if (m_buffer.length())
        {
        wxFile logFile(m_logFilePath, wxFile::write_append);
        if (logFile.IsOpened())
            {
            logFile.Write(m_buffer);
            m_buffer.clear();
            }
        }
    }

//--------------------------------------------------
void LogFile::DoLogTextAtLevel(wxLogLevel level, const wxString& msg)
    {
    wxString prefix;
    switch (level)
        {
    case wxLOG_Debug:
        [[fallthrough]];
    case wxLOG_Trace:
        // don't expose these for translation;
        // log messages are usually only needed for developers,
        // so translating them causes more problems than it solves
        prefix = L"\U0001F41E Debug: ";
        break;
    case wxLOG_FatalError:
        [[fallthrough]];
    case wxLOG_Error:
        prefix = L"\U00002757 Error: ";
        break;
    case wxLOG_Warning:
        prefix = L"\u26A0 Warning: ";
        break;
    default:
        prefix.clear();
        }
    m_buffer += (prefix + msg);
    }

//--------------------------------------------------
void LogFile::DoLogRecord(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info)
    {
    wxString prefix;
    switch (level)
        {
    case wxLOG_Debug:
        [[fallthrough]];
    case wxLOG_Trace:
        prefix = L"\U0001F41E Debug: ";
        break;
    case wxLOG_FatalError:
        [[fallthrough]];
    case wxLOG_Error:
        prefix = L"\U00002757 Error: ";
        break;
    case wxLOG_Warning:
        prefix = L"\u26A0 Warning: ";
        break;
    default:
        prefix.clear();
        }
    m_buffer += wxString::Format(
        L"%s%s\t%s\t%s\t%s: line %d\n", prefix, msg,
        wxDateTime(static_cast<wxLongLong>(info.timestampMS)).FormatISOCombined(' '),
        (info.func ? wxString(info.func) : wxString(L"N/A")),
        (info.filename ? wxFileName(info.filename).GetFullName() : wxString(L"N/A")), info.line);
    }
