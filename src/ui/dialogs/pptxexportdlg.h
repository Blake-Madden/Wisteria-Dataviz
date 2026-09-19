/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_PPTX_EXPORT_DLG_H
#define WISTERIA_PPTX_EXPORT_DLG_H

#include "../../reporting/pptxreportprintout.h"
#include "dialogwithhelp.h"
#include <wx/spinctrl.h>

namespace Wisteria::UI
    {
    /** @brief Options dialog for exporting to PowerPoint.
        @details Prompts the user for slide size, document metadata, an optional
            title slide (with a named color theme), slide transitions, auto-advance/loop
            behavior, and speaker notes.*/
    class PptxExportDlg final : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param options The PowerPoint export options.
            @param caption The title of the export dialog.*/
        PptxExportDlg(wxWindow* parent, PowerPointExportOptions options,
                      const wxString& caption = _(L"PowerPoint Export Options"));

        /// @private
        PptxExportDlg(const PptxExportDlg&) = delete;
        /// @private
        PptxExportDlg& operator=(const PptxExportDlg&) = delete;

        /// @returns The PowerPoint options selected by the user.
        [[nodiscard]]
        const PowerPointExportOptions& GetOptions() const noexcept
            {
            return m_options;
            }

      private:
        void CreateControls();
        void UpdateSlideSizeControls();
        void UpdateTransitionControls();
        void UpdateTitleSlideControls();

        bool Validate() final;

        PowerPointExportOptions m_options;

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
        wxCheckBox* m_advanceOnClickCheck{ nullptr };
        wxCheckBox* m_advanceAutomaticallyCheck{ nullptr };
        wxSpinCtrl* m_advanceSecondsCtrl{ nullptr };
        wxCheckBox* m_loopCheck{ nullptr };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // WISTERIA_PPTX_EXPORT_DLG_H
