/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef PROJECT_SETTINGS_DIALOG_H
#define PROJECT_SETTINGS_DIALOG_H

#include "../../reporting/reportbuilder.h"
#include "dialogwithhelp.h"
#include <wx/clrpicker.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Dialog for editing project-level settings.
    class ProjectSettingsDlg final : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param id The window ID.
            @param caption The dialog caption.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style.*/
        explicit ProjectSettingsDlg(wxWindow* parent, wxWindowID id = wxID_ANY,
                                    const wxString& caption = _(L"Project Settings"),
                                    const wxPoint& pos = wxDefaultPosition,
                                    const wxSize& size = wxDefaultSize,
                                    long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                                 wxRESIZE_BORDER);

        /// @private
        ProjectSettingsDlg() = delete;
        /// @private
        ProjectSettingsDlg(const ProjectSettingsDlg&) = delete;
        /// @private
        ProjectSettingsDlg& operator=(const ProjectSettingsDlg&) = delete;

        /// @brief Pre-fills the dialog from an existing report builder.
        /// @param reportBuilder The report builder to read values from.
        void LoadFromProject(const Wisteria::ReportBuilder& reportBuilder);

        /// @returns The project name entered by the user.
        [[nodiscard]]
        wxString GetProjectName()
            {
            TransferDataFromWindow();
            return m_projectName;
            }

        /// @returns The project subject entered by the user.
        [[nodiscard]]
        wxString GetSubject()
            {
            TransferDataFromWindow();
            return m_subject;
            }

        /// @returns The project keywords entered by the user.
        [[nodiscard]]
        wxString GetKeywords()
            {
            TransferDataFromWindow();
            return m_keywords;
            }

        /// @returns The watermark label entered by the user.
        [[nodiscard]]
        wxString GetWatermarkLabel()
            {
            TransferDataFromWindow();
            return m_watermarkLabel;
            }

        /// @returns The watermark color selected by the user.
        [[nodiscard]]
        wxColour GetWatermarkColor() const
            {
            return (m_watermarkColorPicker != nullptr) ? m_watermarkColorPicker->GetColour() :
                                                         m_watermarkColor;
            }

        /// @returns The left printer header.
        [[nodiscard]]
        const wxString& GetLeftPrinterHeader() const noexcept
            {
            return m_leftPrinterHeader;
            }

        /// @returns The center printer header.
        [[nodiscard]]
        const wxString& GetCenterPrinterHeader() const noexcept
            {
            return m_centerPrinterHeader;
            }

        /// @returns The right printer header.
        [[nodiscard]]
        const wxString& GetRightPrinterHeader() const noexcept
            {
            return m_rightPrinterHeader;
            }

        /// @returns The left printer footer.
        [[nodiscard]]
        const wxString& GetLeftPrinterFooter() const noexcept
            {
            return m_leftPrinterFooter;
            }

        /// @returns The center printer footer.
        [[nodiscard]]
        const wxString& GetCenterPrinterFooter() const noexcept
            {
            return m_centerPrinterFooter;
            }

        /// @returns The right printer footer.
        [[nodiscard]]
        const wxString& GetRightPrinterFooter() const noexcept
            {
            return m_rightPrinterFooter;
            }

      private:
        void CreateControls();
        void OnEditHeadersFooters(wxCommandEvent& event);

        wxColourPickerCtrl* m_watermarkColorPicker{ nullptr };

        wxString m_leftPrinterHeader;
        wxString m_centerPrinterHeader;
        wxString m_rightPrinterHeader;
        wxString m_leftPrinterFooter;
        wxString m_centerPrinterFooter;
        wxString m_rightPrinterFooter;

        wxString m_projectName;
        wxString m_subject;
        wxString m_keywords;
        wxString m_watermarkLabel;
        wxColour m_watermarkColor{ wxColour(255, 0, 0) };
        };
    } // namespace Wisteria::UI

/// @}

#endif // PROJECT_SETTINGS_DIALOG_H
