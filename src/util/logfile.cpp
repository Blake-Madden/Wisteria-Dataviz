///////////////////////////////////////////////////////////////////////////////
// Name:        memorymappedfile.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "logfile.h"

//--------------------------------------------------
LogFile::LogFile()
    {
    // will be a unique file name on a per day basis
    m_logFilePath = wxStandardPaths::Get().GetTempDir() + wxFileName::GetPathSeparator() +
        wxTheApp->GetAppName() + wxDateTime::Now().FormatISODate() + L".log";
    wxFile logFile;
    if (!logFile.Create(m_logFilePath, true))
        {
        wxMessageBox(wxString::Format(
            _("Unable to create log file at '%s'"), m_logFilePath),
            _("Logging Error"), wxOK|wxICON_WARNING);
        }
    else
        {
        // clear file (from a previous program run) and prepare for appending
        logFile.Write(wxEmptyString);
        }
    }

//--------------------------------------------------
wxString LogFile::ReadLog()
    {
    LogFile::Flush();

    wxString logBuffer;
    wxFile logFile(m_logFilePath, wxFile::read);
    if (logFile.IsOpened())
        { logFile.ReadAll(&logBuffer); }
    // flushing to temp file failed somehow,
    // so return whatever is queued up at least
    else
        { return m_buffer; }
    return logBuffer;
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
void LogFile::DoLogTextAtLevel(wxLogLevel level, const wxString &msg)
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
            prefix = L"\x26A0 Warning: ";
            break;
        default:
            prefix.clear();
        }
    m_buffer += (prefix + msg);
    }

//--------------------------------------------------
void LogFile::DoLogRecord(wxLogLevel level, const wxString &msg,
                          const wxLogRecordInfo &info)
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
            prefix = L"\x26A0 Warning: ";
            break;
        default:
            prefix.clear();
        }
    m_buffer += wxString::Format(L"%s%s\t%s\t%s\t%s: line %d\n",
        prefix, msg,
        wxDateTime(info.timestamp).FormatISOCombined(' '),
        (info.func ? wxString(info.func) : L"N/A"),
        (info.filename ? wxFileName(info.filename).GetFullName() : L"N/A"),
        info.line);
    }
