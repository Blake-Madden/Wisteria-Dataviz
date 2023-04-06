/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __LOGFILLE_H__
#define __LOGFILLE_H__

#include <wx/wx.h>
#include <wx/string.h>
#include <wx/log.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

/** @brief Logging system that writes its records to a temp file.
    @details Each record in the log report is highly verbose. It will include
     the log level, message, timestamp, and location (function, file, and line #)
     of the call to @c wxLog.
    @par Example:
    @code
     // In your app's OnInit(), call this and then logging will
     // go to a file in your temp folder.
     // (This file will be named [APP NAME] + the current date.)
     auto logFile = new LogFile;
     delete wxLog::SetActiveTarget(logFile);

     // at any time, call this to read the log file:
     wxString logMessages = logFile->ReadLog();
    @endcode
    
    @par Log Format
     The log file is recorded as a tab-delimited text file with the
     following format (with example content):
     
     | Log Level                   | Message                    | Timestamp            | Function Name      | Filename        | Line |
     | :--                         | :--                        | :--                  | :--                | :--             | :--  |
     | @emoji :warning: Warning:   | System font name not found | 2022-02-27T08:32:47  | @c LoadFonts()     | fontloader.cpp  | 122  |
     | @emoji :exclamation: Error: | Invalid serial number!     | 2022-02-27T08:33:05  | @c GetUserInfo()   | userinfo.cpp    | 476  |

     Note that icons/emojis are included in the first column of the
     log report to help visualize the log level.

     | Icon                 | Log Level |
     | :--                  |           |
     | @emoji :warning:     | Warning   |
     | @emoji :exclamation: | Error     |
     | @emoji :bug:         | Debug     |
    */
class LogFile : public wxLog
    {
public:
    /// @brief Default constructor.
    /// @details Should be created on the heap and passed to
    ///     @c wxLog::SetActiveTarget().
    LogFile();
    /// @private
    LogFile(const LogFile&) = delete;
    /// @private
    LogFile(LogFile&&) = delete;
    /// @private
    LogFile& operator=(const LogFile&) = delete;
    /// @private
    LogFile& operator=(LogFile&&) = delete;

    /// @brief Reads (and returns) the content of the log file.
    /// @returns The content of the log report.
    [[nodiscard]]
    wxString ReadLog();

    /** @brief Gets the path of the log file.
        @details This can be useful for archiving a log file
         when your program exits.\n
         This can also be used for adding the log file to a crash report:
        @code
         wxDebugReportCompress* report = new wxDebugReportCompress;
         //"logFile" will the LogFile object you created earlier
         report->AddFile(logFile->GetLogFilePath(), _(L"Log Report"));
         ...
        @endcode
        @returns The path of the log file.
     */
    [[nodiscard]]
    const wxString& GetLogFilePath() const noexcept
        { return m_logFilePath; }

    /// @private
    void Flush() final;
protected:
    /// @private
    void DoLogText(const wxString &msg) final
        { m_buffer << msg << L"\n"; }
    /// @private
    void DoLogRecord(wxLogLevel level, const wxString &msg,
                     const wxLogRecordInfo &info) final;
    /// @private
    void DoLogTextAtLevel(wxLogLevel level, const wxString &msg) final;
private:
    wxString m_buffer;
    wxString m_logFilePath;
    };

/** @}*/

#endif //__LOGFILLE_H__
