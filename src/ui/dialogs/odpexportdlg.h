/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_ODP_EXPORT_DLG_H
#define WISTERIA_ODP_EXPORT_DLG_H

#include "../../reporting/odpreportprintout.h"
#include "dialogwithhelp.h"
#include <wx/spinctrl.h>

namespace Wisteria::UI
    {
    /// @brief Options dialog for exporting to LibreOffice Impress (@c .odp).
    class OdpExportDlg final : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param options The ODP export options.
            @param caption The title of the export dialog.*/
        OdpExportDlg(wxWindow* parent, OdpExportOptions options,
                     const wxString& caption = _(L"ODP Export Options"));

        /// @private
        OdpExportDlg(const OdpExportDlg&) = delete;
        /// @private
        OdpExportDlg& operator=(const OdpExportDlg&) = delete;

        /// @returns The ODP options selected by the user.
        [[nodiscard]]
        const OdpExportOptions& GetOptions() const noexcept
            {
            return m_options;
            }

      private:
        void CreateControls();
        void UpdateSlideSizeControls();
        void UpdateTransitionControls();
        void UpdateTitleSlideControls();

        bool Validate() final;

        OdpExportOptions m_options;

        wxRadioBox* m_slideSizeRadio{ nullptr };
        wxStaticText* m_customWidthLabel{ nullptr };
        wxSpinCtrlDouble* m_customWidthCtrl{ nullptr };
        wxStaticText* m_customHeightLabel{ nullptr };
        wxSpinCtrlDouble* m_customHeightCtrl{ nullptr };

        wxCheckBox* m_titleSlideCheck{ nullptr };
        wxStaticText* m_titleSlideThemeLabel{ nullptr };
        wxChoice* m_titleSlideThemeChoice{ nullptr };

        wxChoice* m_transitionChoice{ nullptr };
        wxStaticText* m_transitionSpeedLabel{ nullptr };
        wxChoice* m_transitionSpeedChoice{ nullptr };
        wxCheckBox* m_advanceAutomaticallyCheck{ nullptr };
        wxSpinCtrl* m_advanceSecondsCtrl{ nullptr };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // WISTERIA_ODP_EXPORT_DLG_H
