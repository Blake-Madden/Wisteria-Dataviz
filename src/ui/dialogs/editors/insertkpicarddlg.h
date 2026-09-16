/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_KPI_CARD_DIALOG_H
#define INSERT_KPI_CARD_DIALOG_H

#include "insertlabeldlg.h"
#include <wx/clrpicker.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Simplified dialog for inserting a "KPI card" style Label
            (a big number with a caption).
        @details Reuses InsertLabelDlg's "Shapes" and "Placement" pages as-is,
            replacing its general-purpose "Label" page with a small "KPI Card"
            page (big number, caption, number color, layout preset).

            This dialog is insert-only; editing an already-placed KPI card
            uses the regular InsertLabelDlg.*/
    class InsertKpiCardDlg final : public InsertLabelDlg
        {
      public:
        /** @brief Constructor.
            @param canvas The canvas whose grid layout is displayed.
            @param reportBuilder The report builder (may be @c nullptr).
            @param parent The parent window.
            @param caption The dialog title.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.*/
        InsertKpiCardDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                         const wxString& caption = _(L"Insert KPI Card"), wxWindowID id = wxID_ANY,
                         const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                         long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);

        /// @private
        InsertKpiCardDlg(const InsertKpiCardDlg&) = delete;
        /// @private
        InsertKpiCardDlg& operator=(const InsertKpiCardDlg&) = delete;

        /// @returns A new KPI-card label built from the dialog's current settings.
        /// @note Call after ShowModal() returns wxID_OK.
        [[nodiscard]]
        std::shared_ptr<Wisteria::GraphItems::Label> BuildKpiCard();

      private:
        void CreateControls() final;
        void CreateKpiPage();

        // starts at +4 to avoid collision with InsertLabelDlg's
        // ID_LABEL_SECTION (+2) and ID_SHAPES_SECTION (+3)
        constexpr static wxWindowID ID_KPI_SECTION{ wxID_HIGHEST + 4 };

        // KPI content
        wxTextCtrl* m_bigNumberCtrl{ nullptr }; // e.g., "1,204", "$45.2K", "-5%"
        wxTextCtrl* m_captionCtrl{ nullptr };   // e.g., "Fall Enrollment", "Less than target"
        wxColourPickerCtrl* m_numberColorPicker{ nullptr };
        wxColourPickerCtrl* m_captionColorPicker{ nullptr };

        // alignments (in TextAlignment order)
        int m_numberAlignment{ 2 };  // Centered
        int m_captionAlignment{ 2 }; // Centered

        // layout preset
        int m_layoutPreset{ 0 };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_KPI_CARD_DIALOG_H
