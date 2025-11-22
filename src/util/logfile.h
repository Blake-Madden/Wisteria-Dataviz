/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_LOGFILLE_H
#define WISTERIA_LOGFILLE_H

#include <wx/log.h>
#include <wx/string.h>

// clang-format off
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
// clang-format on

class LogFile final : public wxLog
    {
  public:
    /// @brief Default constructor.
    /// @param clearPreviousLog @c true to clear the contents of the target
    ///     log file if it exists. @c false is recommended if you wish to
    ///     preserve the contents of a log file from a previous run of a
    ///     program (as an example). @c true is recommended if you wish to
    ///     have a fresh log file when activating this logger.
    /// @details Should be created on the heap and passed to
    ///     @c wxLog::SetActiveTarget().
    explicit LogFile(bool clearPreviousLog);

    /// @private
    LogFile(const LogFile&) = delete;

    /// @private
    LogFile& operator=(const LogFile&) = delete;

    /// @brief Reads (and returns) the content of the log file.
    /// @returns The content of the log report.
    [[nodiscard]]
    wxString Read();

    /** @brief Gets the path of the log file.
        @details This can be useful for archiving a log file
         when your program exits.\n
         This can also be used for adding the log file to a crash report:
        @code
         wxDebugReportCompress* report = new wxDebugReportCompress;
         // "logFile" will the LogFile object you created earlier
         report->AddFile(logFile->GetLogFilePath(), _(L"Log Report"));
         ...
        @endcode
        @returns The path of the log file.
     */
    [[nodiscard]]
    const wxString& GetLogFilePath() const noexcept
        {
        return m_logFilePath;
        }

    /// @brief Clears the contents of the log file.
    /// @note Flush will not be called, so pending messages will still
    ///     be queued for processing. Call Flush prior to calling this
    ///     if you wish to delete any queued messages.
    /// @returns @c true if the log file was successfully cleared.
    bool Clear();

    /// @private
    void Flush() final;

  protected:
    /// @private
    void DoLogText(const wxString& msg) final { m_buffer << msg << L"\n"; }

    /// @private
    void DoLogRecord(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info) final;

    /// @private
    void DoLogTextAtLevel(wxLogLevel level, const wxString& msg) final;

  private:
    wxString m_buffer;
    wxString m_logFilePath;
    };

    /** @}*/

#endif // WISTERIA_LOGFILLE_H
