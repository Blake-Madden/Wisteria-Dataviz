/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef SVG_EXPORT_DLG_H
#define SVG_EXPORT_DLG_H

#include "../../math/mathematics.h"
#include <wx/dialog.h>
#include <wx/panel.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Options dialog for exporting an SVG report.
    /// @details Prompts the user for the page width and height (in pixels)
    ///     and shows a small aspect-ratio preview panel.
    class SvgExportDlg final : public wxDialog
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param defaultSize The default page size (in DIPs/pixels) to
                pre-populate the spin controls with.
            @param id The window ID.
            @param caption The title of the export dialog.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        SvgExportDlg(wxWindow* parent, const wxSize& defaultSize, wxWindowID id = wxID_ANY,
                     const wxString& caption = _(L"SVG Export Options"),
                     const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                     long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN);

        /// @private
        SvgExportDlg(const SvgExportDlg&) = delete;
        /// @private
        SvgExportDlg& operator=(const SvgExportDlg&) = delete;

        /// @returns The page size selected by the user (in DIPs/pixels).
        [[nodiscard]]
        wxSize GetPageSize() const noexcept
            {
            return { m_pageWidth, m_pageHeight };
            }

        /// @returns Whether to include smooth transitions.
        [[nodiscard]]
        bool IncludeTransitions() const noexcept
            {
            return m_includeTransitions;
            }

        /// @returns Whether to include interactive highlighting.
        [[nodiscard]]
        bool IncludeHighlighting() const noexcept
            {
            return m_includeHighlighting;
            }

        /// @returns Whether to include a layout toggle.
        [[nodiscard]]
        bool IncludeLayoutToggle() const noexcept
            {
            return m_includeLayoutToggle;
            }

        /// @returns The selected button color.
        [[nodiscard]]
        wxColour GetButtonColor() const noexcept
            {
            return m_buttonColor;
            }

        /// @returns The selected horizontal gap (in pixels).
        [[nodiscard]]
        int GetHorizontalGap() const noexcept
            {
            return m_horizontalGap;
            }

      private:
        void CreateControls();
        void OnSizeChanged(wxSpinEvent& event);
        void OnPaintPreview(wxPaintEvent& event);
        void UpdatePreview();

        void OnOK([[maybe_unused]] wxCommandEvent& event)
            {
            TransferDataFromWindow();
            if (IsModal())
                {
                EndModal(wxID_OK);
                }
            else
                {
                Show(false);
                }
            }

        constexpr static wxWindowID PAGE_WIDTH_ID{ wxID_HIGHEST };
        constexpr static wxWindowID PAGE_HEIGHT_ID{ wxID_HIGHEST + 1 };
        constexpr static wxWindowID BUTTON_COLOR_ID{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID HORIZONTAL_GAP_ID{ wxID_HIGHEST + 3 };

        int m_pageWidth{ 0 };
        int m_pageHeight{ 0 };
        bool m_includeTransitions{ true };
        bool m_includeHighlighting{ true };
        bool m_includeLayoutToggle{ true };
        wxColour m_buttonColor{ 103, 58, 183 };
        int m_horizontalGap{ 25 };

        wxPanel* m_previewPanel{ nullptr };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // SVG_EXPORT_DLG_H
