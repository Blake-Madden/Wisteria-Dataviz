/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_BASEAPP_H
#define WISTERIA_BASEAPP_H

#include "../debug/debug_profile.h"
#include "../math/safe_math.h"
#include "../util/donttranslate.h"
#include "../util/logfile.h"
#include "../util/resource_manager.h"
#include "mainframe.h"
#include <wx/cmdline.h>
#include <wx/config.h>
#include <wx/debugrpt.h>
#include <wx/docview.h>
#include <wx/fileconf.h>
#include <wx/fs_mem.h>
#include <wx/fs_zip.h>
#include <wx/image.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/mimetype.h>
#include <wx/print.h>
#include <wx/printdlg.h>
#include <wx/propgrid/propgrid.h>
#include <wx/splash.h>
#include <wx/stdpaths.h>
#include <wx/sysopt.h>
#include <wx/uilocale.h>
#include <wx/utils.h>
#include <wx/webrequest.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Application class with file history, file logger, profiler,
    ///     exception handling, document manager, and ribbon-based main frame built-in.
    class BaseApp : public wxApp
        {
      public:
        /// @brief Constructor.
        BaseApp();
        /// @private
        BaseApp(const BaseApp&) = delete;
        /// @private
        BaseApp& operator=(const BaseApp&) = delete;

        /// @private
        bool OnInit() override;
        /// @private
        int OnExit() override;

        /// @brief Logs various information about the system.
        /// @details Call this after OnInit().
        void LogSystemInfo() const;

        /// @private
        void OnFatalException() final;
        // this is where we really generate the debug report
        /// @private
        void GenerateReport(wxDebugReport::Context ctx) const;

        /// @returns The number of active documents.
        [[nodiscard]]
        size_t GetDocumentCount() const
            {
            return m_docManager->GetDocuments().GetCount();
            }

        /// @returns The application's main file extension.
        [[nodiscard]]
        const wxString& GetAppFileExtension() const noexcept
            {
            return m_fileExtension;
            }

        /// @brief Sets the application's main file extension.
        /// @param extension The file extension to use.
        void SetAppFileExtension(const wxString& extension) { m_fileExtension = extension; }

        // MRU menu functions
        /// @private
        void LoadFileHistoryMenu();
        /// @private
        void SaveFileHistoryMenu();
        /// @brief Clears the MRU file history.
        virtual void ClearFileHistoryMenu();

        /// @returns The mainframe.
        [[nodiscard]]
        BaseMainFrame* GetMainFrame() noexcept
            {
            return m_mainFrame;
            }

        /// @private
        [[nodiscard]]
        const BaseMainFrame* GetMainFrame() const noexcept
            {
            return m_mainFrame;
            }

        /// @brief Sets the mainframe.
        /// @param frame The mainframe to use.
        void SetMainFrame(BaseMainFrame* frame)
            {
            m_mainFrame = frame;
            SetTopWindow(m_mainFrame);
            }

        /// @returns The document manager.
        [[nodiscard]]
        wxDocManager* GetDocManager() noexcept
            {
            return m_docManager;
            }

        /// @brief Sets the document manager.
        /// @param docManager The document manager to use.
        void SetDocManager(wxDocManager* docManager) noexcept { m_docManager = docManager; }

        /// @brief Gets a window that is suitable for parenting a dialog.
        /// @details Will use the mainframe if visible; otherwise,
        ///     the current doc window will be used (if visible).
        ///     Then goes through the rest of the documents until it finds one that is visible.
        ///     If no document window is available and visible, then falls back
        ///     to the top-level window.
        ///     Visibility is necessary for parenting under Linux.
        /// @returns A window suitable for parenting a dialog.
        [[nodiscard]]
        wxWindow* GetParentingWindow();

        /// @brief Sets a descriptive name for the application's document type.
        /// @param documentTypeName The descriptions.
        void SetDocumentTypeName(const wxString& documentTypeName)
            {
            m_documentTypeName = documentTypeName;
            }

        /// @returns The application's document version number.
        [[nodiscard]]
        const wxString& GetDocumentVersionNumber() const noexcept
            {
            return m_documentVersionNumber;
            }

        /// @brief Sets the application's document version number.
        /// @param versionNumber The version number.
        void SetDocumentVersionNumber(const wxString& versionNumber)
            {
            m_documentVersionNumber = versionNumber;
            }

        /// @returns The application's subname.
        [[nodiscard]]
        const wxString& GetAppSubName() const noexcept
            {
            return m_appSubName;
            }

        /// @brief Sets the application's subname.
        /// @param name The subname.
        void SetAppSubName(const wxString& name) { m_appSubName = name; }

        /// @returns The path of where the debug profiling data is being saved.
        /// @details This is only used if profiling is enabled.
        [[nodiscard]]
        const wxString& GetProfileReportPath() const noexcept
            {
            return m_profileReportPath;
            }

        /// @returns Everything sent to the logging system as a formatted string.
        [[nodiscard]]
        wxString GetLogReport() const
            {
            return m_logFile->Read();
            }

        /// @return The file logging systems used by the application.
        [[nodiscard]]
        LogFile* GetLogFile() const noexcept
            {
            return m_logFile;
            }

        /// @returns @c true if the daily log file is being appended to
        ///     when the program starts. @c false indicates that it will
        ///     be overwritten.
        [[nodiscard]]
        bool IsAppendingDailyLog() const noexcept
            {
            return m_appendDailyLog;
            }

        /// @brief Specifies whether the log from the same day should be
        ///     appended to or overwritten when the application starts.
        /// @param append @c true to append to the log file from today;
        ///     @c false to overwrite it.
        /// @note This must be called before BaseApp::OnInit().
        void AppendDailyLog(const bool append) noexcept { m_appendDailyLog = append; }

        /// @brief Sets a support email for the application.
        /// @param email The email address.
        void SetSupportEmail(const wxString& email) { m_supportEmail = email; }

        /// @returns The printer settings.
        [[nodiscard]]
        wxPrintData* GetPrintData()
            {
            if (GetMainFrame() != nullptr)
                {
                return &GetMainFrame()
                            ->GetDocumentManager()
                            ->GetPageSetupDialogData()
                            .GetPrintData();
                }
            return nullptr;
            }

        /// @returns The path to a file in the application's resource directory,
        ///     or empty string if not found.
        /// @param subFile The file to look for.
        [[nodiscard]]
        wxString FindResourceFile(const wxString& subFile) const;

        /// @returns A subdirectory's full path in the application's resource directory,
        ///     or empty string if not found.
        /// @param subDir The subdirectory to look for.
        [[nodiscard]]
        wxString FindResourceDirectory(const wxString& subDir) const;

        /// @returns The resource manager, which can extract images and
        ///     XRC files from a resource archive.
        [[nodiscard]]
        ResourceManager& GetResourceManager() noexcept
            {
            return m_resManager;
            }

        /// @private
        [[nodiscard]]
        const ResourceManager& GetResourceManager() const noexcept
            {
            return m_resManager;
            }

        // @brief Loads a bitmap (with provided size in DIPs) from provided path to be compatible
        //      with the UI.
        // @details This will handle DIPs and scale factor (i.e., Retina display) calculations.
        wxBitmap ReadSvgIcon(const wxString& path, const wxSize baseSize = wxSize{ 32, 32 });

        /** @brief Creates a program's splashscreen using a base image and
                various program information.
            @param bitmap The base image.
            @param appName The application's name.
            @param appSubName The application's supplemental name
                (e.g., the version number).
            @param vendorName The application's vendor.
            @param includeCopyright Whether to show a copyright label at the
                bottom of the splashscreen.
            @param copyrightPrefix A string to insert at the start of the copyright message.
            @returns The decorated splashscreen.*/
        [[nodiscard]]
        static wxBitmap CreateSplashscreen(const wxBitmap& bitmap, const wxString& appName,
                                           const wxString& appSubName, const wxString& vendorName,
                                           const bool includeCopyright,
                                           const wxString& copyrightPrefix = wxString{});

      private:
        [[nodiscard]]
        wxString FindResourceFileWithAppInfo(const wxString& folder, const wxString& subFile) const;
        [[nodiscard]]
        wxString FindResourceDirectoryWithAppInfo(const wxString& folder,
                                                  const wxString& subFolder) const;

        wxDocManager* m_docManager{ nullptr };
        Wisteria::UI::BaseMainFrame* m_mainFrame{ nullptr };
        LogFile* m_logFile{ nullptr };
        bool m_appendDailyLog{ false };
        ResourceManager m_resManager;
        wxLocale* m_locale{ nullptr };

        wxString m_appSubName;
        wxString m_fileExtension;
        wxString m_documentTypeName;
        wxString m_documentVersionNumber;
        wxString m_profileReportPath;
        wxString m_supportEmail;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // WISTERIA_BASEAPP_H
