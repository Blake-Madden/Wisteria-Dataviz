/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_SHAPE_DIALOG_H
#define INSERT_SHAPE_DIALOG_H

#include "../../base/fillableshape.h"
#include "insertitemdlg.h"
#include <wx/clrpicker.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting or editing a Shape or FillableShape on a canvas cell.
        @details Extends InsertItemDlg with a "Shape Options" page containing:
            - Shape type (via choice control with human-readable names).
            - Size (width and height in DIPs).
            - Pen (color, width, style).
            - Brush (color, style).
            - Label text and font color.
            - Whether the shape is fillable, and if so, the fill percent.

        Can also be used to edit an existing Shape or FillableShape by calling
        LoadFromShape() or LoadFromFillableShape() before showing the dialog.*/
    class InsertShapeDlg final : public InsertItemDlg
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
            @param editMode Whether the item is being inserted or edited.*/
        InsertShapeDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                       const wxString& caption = _(L"Insert Shape"), wxWindowID id = wxID_ANY,
                       const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                       long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                       EditMode editMode = EditMode::Insert);

        /// @private
        InsertShapeDlg(const InsertShapeDlg&) = delete;
        /// @private
        InsertShapeDlg& operator=(const InsertShapeDlg&) = delete;

        /// @returns The selected icon shape.
        [[nodiscard]]
        Icons::IconShape GetIconShape() const noexcept;

        /// @returns The shape width (in DIPs).
        [[nodiscard]]
        int GetShapeWidth() const;

        /// @returns The shape height (in DIPs).
        [[nodiscard]]
        int GetShapeHeight() const;

        /// @returns The pen color.
        [[nodiscard]]
        wxColour GetPenColor() const;

        /// @returns The pen width.
        [[nodiscard]]
        int GetPenWidth() const;

        /// @returns The pen style.
        [[nodiscard]]
        wxPenStyle GetPenStyle() const noexcept;

        /// @returns The brush color.
        [[nodiscard]]
        wxColour GetBrushColor() const;

        /// @returns The brush style.
        [[nodiscard]]
        wxBrushStyle GetBrushStyle() const noexcept;

        /// @returns The label text.
        [[nodiscard]]
        wxString GetLabelText() const;

        /// @returns The label font color.
        [[nodiscard]]
        wxColour GetLabelFontColor() const;

        /// @returns The selected horizontal page alignment.
        [[nodiscard]]
        PageHorizontalAlignment GetHorizontalAlignment() const noexcept;

        /// @returns The selected vertical page alignment.
        [[nodiscard]]
        PageVerticalAlignment GetVerticalAlignment() const noexcept;

        /// @returns @c true if fillable mode is enabled.
        [[nodiscard]]
        bool IsFillable() const noexcept
            {
            return m_fillable;
            }

        /// @returns The fill percent (0.0 to 1.0).
        [[nodiscard]]
        double GetFillPercent() const;

        /// @brief Populates controls from an existing Shape.
        /// @param shape The shape to read settings from.
        /// @param canvas The canvas the shape belongs to.
        void LoadFromShape(const Wisteria::GraphItems::Shape& shape, Canvas* canvas);

        /// @brief Populates controls from an existing FillableShape.
        /// @param shape The fillable shape to read settings from.
        /// @param canvas The canvas the shape belongs to.
        void LoadFromFillableShape(const Wisteria::GraphItems::FillableShape& shape,
                                   Canvas* canvas);

        /// @brief Populates controls from a ShapeInfo.
        /// @param shapeInfo The shape info to read settings from.
        void LoadFromShapeInfo(const Wisteria::GraphItems::ShapeInfo& shapeInfo);

      private:
        void CreateControls() final;
        bool Validate() final;
        void OnEnableFillable(bool enable);

        /// @brief Populates the shape choice with human-readable names.
        void PopulateShapeChoice();

        // starts at +4 to avoid collision with InsertItemDlg (+1),
        // InsertLabelDlg (+2), InsertImageDlg (+3)
        constexpr static wxWindowID ID_SHAPE_SECTION{ wxID_HIGHEST + 4 };

        // shape selection
        wxChoice* m_shapeChoice{ nullptr };

        // size
        wxSpinCtrl* m_widthSpin{ nullptr };
        wxSpinCtrl* m_heightSpin{ nullptr };

        // pen
        wxColourPickerCtrl* m_penColorPicker{ nullptr };
        wxSpinCtrl* m_penWidthSpin{ nullptr };

        // brush
        wxColourPickerCtrl* m_brushColorPicker{ nullptr };

        // label
        wxTextCtrl* m_labelTextCtrl{ nullptr };
        wxColourPickerCtrl* m_labelColorPicker{ nullptr };

        // fillable
        wxSpinCtrlDouble* m_fillPercentSpin{ nullptr };

        // DDX data members
        int m_shapeIndex{ 10 }; // default to Square
        int m_penStyle{ 0 };
        int m_brushStyle{ 0 };
        bool m_fillable{ false };
        int m_horizontalAlign{ 1 }; // Centered
        int m_verticalAlign{ 1 };   // Centered

        // mapping from choice index to IconShape
        std::vector<Icons::IconShape> m_shapeMap;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_SHAPE_DIALOG_H
