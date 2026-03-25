/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_LABEL_DIALOG_H
#define INSERT_LABEL_DIALOG_H

#include "../../base/shapes.h"
#include "../controls/thumbnail.h"
#include "insertitemdlg.h"
#include <wx/clrpicker.h>
#include <wx/editlbox.h>
#include <wx/fontpicker.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting or editing a Label on a canvas cell.
        @details Extends InsertItemDlg with a "Label" page containing:
            - Multi-line text.
            - Font (via font picker).
            - Font color and background color (via color pickers).
            - Text alignment (Left, Center, Right).
            - Header options (treat first line as header, with its own

        Can also be used standalone to edit an existing Label's styling
        (e.g., the donut hole label in a pie chart) by calling
        LoadFromLabel() before showing the dialog and ApplyToLabel()
        after it returns wxID_OK.*/
    class InsertLabelDlg final : public InsertItemDlg
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
            @param style The window style.
            @param editMode Whether the item is being inserted or edited.
            @param includePageOptions Whether to show the "Page Options" section.
                Set to @c false when the label is not placed directly on the canvas
                (e.g., a donut hole label inside a pie chart).*/
        InsertLabelDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                       const wxString& caption = _(L"Insert Label"), wxWindowID id = wxID_ANY,
                       const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                       long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                       EditMode editMode = EditMode::Insert, bool includePageOptions = true);

        /// @private
        InsertLabelDlg(const InsertLabelDlg&) = delete;
        /// @private
        InsertLabelDlg& operator=(const InsertLabelDlg&) = delete;

        /// @returns The label text.
        [[nodiscard]]
        wxString GetLabelText() const
            {
            return m_textCtrl != nullptr ? m_textCtrl->GetValue() : wxString{};
            }

        /// @returns The selected font.
        [[nodiscard]]
        wxFont GetLabelFont() const
            {
            return m_fontPicker != nullptr ? m_fontPicker->GetSelectedFont() :
                                             wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
            }

        /// @returns The font color.
        [[nodiscard]]
        wxColour GetFontColor() const
            {
            return m_fontColorPicker != nullptr ? m_fontColorPicker->GetColour() : *wxBLACK;
            }

        /// @returns The background color.
        [[nodiscard]]
        wxColour GetBackgroundColor() const
            {
            return m_bgColorPicker != nullptr ? m_bgColorPicker->GetColour() : wxTransparentColour;
            }

        /// @returns The text alignment.
        [[nodiscard]]
        TextAlignment GetLabelAlignment() const noexcept
            {
            return static_cast<TextAlignment>(m_alignment);
            }

        /// @returns @c true if the header is enabled.
        [[nodiscard]]
        bool IsHeaderEnabled() const noexcept
            {
            return m_headerEnabled;
            }

        /// @returns The header font.
        [[nodiscard]]
        wxFont GetHeaderFont() const
            {
            return m_headerFontPicker != nullptr ?
                       m_headerFontPicker->GetSelectedFont() :
                       wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
            }

        /// @returns The header font color.
        [[nodiscard]]
        wxColour GetHeaderFontColor() const
            {
            return m_headerColorPicker != nullptr ? m_headerColorPicker->GetColour() : *wxBLACK;
            }

        /// @returns The header text alignment.
        [[nodiscard]]
        TextAlignment GetHeaderAlignment() const noexcept
            {
            return static_cast<TextAlignment>(m_headerAlignment);
            }

        /// @returns The header relative scaling.
        [[nodiscard]]
        double GetHeaderScaling() const
            {
            return m_headerScalingSpin != nullptr ? m_headerScalingSpin->GetValue() : 1.0;
            }

        /// @returns The line spacing.
        [[nodiscard]]
        double GetLineSpacing() const
            {
            return m_lineSpacingSpin != nullptr ? m_lineSpacingSpin->GetValue() : 1.0;
            }

        /// @returns The left image file path (empty if none).
        [[nodiscard]]
        const wxString& GetLeftImagePath() const noexcept
            {
            return m_leftImagePath;
            }

        /// @returns The top shapes (may be empty if none are configured).
        [[nodiscard]]
        const std::vector<Wisteria::GraphItems::ShapeInfo>& GetTopShapes() const noexcept
            {
            return m_topShapes;
            }

        /// @returns The top shape offset (in DIPs).
        [[nodiscard]]
        int GetTopShapeOffset() const;

        /// @brief Populates controls from an existing label.
        /// @param label The label to read settings from.
        void LoadFromLabel(const Wisteria::GraphItems::Label& label);

        /// @brief Applies the dialog settings to a label.
        /// @param label The label to update.
        void ApplyToLabel(Wisteria::GraphItems::Label& label) const;

      private:
        void CreateControls() final;
        bool Validate() final;
        void OnEnableHeader(bool enable);

        void OnAddTopShape();
        void OnEditTopShape();
        /// @brief Builds a display string for a ShapeInfo (e.g., "Square (32×32)").
        [[nodiscard]]
        static wxString FormatShapeLabel(const Wisteria::GraphItems::ShapeInfo& shp);
        void RefreshTopShapeList();

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_LABEL_SECTION{ wxID_HIGHEST + 2 };

        bool m_includePageOptions{ true };

        // text
        wxTextCtrl* m_textCtrl{ nullptr };

        // font
        wxFontPickerCtrl* m_fontPicker{ nullptr };
        wxColourPickerCtrl* m_fontColorPicker{ nullptr };
        wxColourPickerCtrl* m_bgColorPicker{ nullptr };

        // DDX data members
        int m_alignment{ 0 }; // FlushLeft
        wxSpinCtrlDouble* m_lineSpacingSpin{ nullptr };

        // header
        bool m_headerEnabled{ false };
        int m_headerAlignment{ 2 }; // Centered
        wxFontPickerCtrl* m_headerFontPicker{ nullptr };
        wxColourPickerCtrl* m_headerColorPicker{ nullptr };
        wxChoice* m_headerAlignmentChoice{ nullptr };
        wxSpinCtrlDouble* m_headerScalingSpin{ nullptr };

        // left image
        Thumbnail* m_leftImageThumbnail{ nullptr };
        wxString m_leftImagePath;

        // top shapes
        std::vector<Wisteria::GraphItems::ShapeInfo> m_topShapes;
        wxEditableListBox* m_topShapeListBox{ nullptr };
        wxSpinCtrl* m_topShapeOffsetSpin{ nullptr };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_LABEL_DIALOG_H
