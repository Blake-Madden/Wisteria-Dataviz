/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_PICTOGRAPH_DIALOG_H
#define INSERT_PICTOGRAPH_DIALOG_H

#include "../../graphs/pictograph.h"
#include "insertgraphdlg.h"
#include <utility>
#include <vector>
#include <wx/clrpicker.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a pictograph into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg for selecting the
              label column and the value column.
            - A "Shape..." button that opens an InsertShapeDlg for selecting the icon shape.
            - A choice for the orientation of the icons.
            - A choice for how values are displayed in the labels.
            - Color pickers for the icons' fill and outline.*/
    class InsertPictographDlg final : public InsertGraphDlg
        {
      public:
        /** @brief Constructor.
            @param canvas The canvas whose grid layout is displayed.
            @param reportBuilder The report builder containing the project's datasets.
            @param parent The parent window.
            @param caption The dialog title.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.
            @param editMode Whether the item is being inserted or edited.*/
        InsertPictographDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                            const wxString& caption = _(L"Insert Pictograph"),
                            wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize,
                            long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                            EditMode editMode = EditMode::Insert);

        /// @private
        InsertPictographDlg(const InsertPictographDlg&) = delete;
        /// @private
        InsertPictographDlg& operator=(const InsertPictographDlg&) = delete;

        /// @returns The selected dataset, or @c nullptr if none.
        [[nodiscard]]
        std::shared_ptr<Data::Dataset> GetSelectedDataset() const;

        /// @returns The name of the selected dataset, or empty if none.
        [[nodiscard]]
        wxString GetSelectedDatasetName() const
            {
            const int sel = m_datasetChoice->GetSelection();
            return (sel != wxNOT_FOUND && std::cmp_less(sel, m_datasetNames.size())) ?
                       m_datasetNames[sel] :
                       wxString{};
            }

        /// @returns The label variable name (drawn beside each icon).
        [[nodiscard]]
        const wxString& GetLabelVariable() const noexcept
            {
            return m_labelVariable;
            }

        /// @returns The value variable name.
        [[nodiscard]]
        const wxString& GetValueVariable() const noexcept
            {
            return m_valueVariable;
            }

        /// @returns The shape used for every icon.
        [[nodiscard]]
        Icons::IconShape GetIconShape() const noexcept
            {
            return m_iconShape;
            }

        /// @returns The orientation of the icons.
        [[nodiscard]]
        Orientation GetOrientation() const noexcept;

        /// @returns How values are formatted in the labels.
        [[nodiscard]]
        NumberDisplay GetValueFormat() const noexcept;

        /// @returns The color that the icons are filled with.
        [[nodiscard]]
        wxColour GetFillColor() const;

        /// @returns The color that the icons are outlined with.
        [[nodiscard]]
        wxColour GetOutlineColor() const;

        /// @brief Populates all dialog controls from an existing pictograph.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

        /** @brief Constructs a pictograph from the dialog's current settings.
            @param oldGraph The previous graph being edited, or @c nullptr if inserting new.
            @returns The newly constructed pictograph.*/
        [[nodiscard]]
        std::shared_ptr<Graphs::Pictograph>
        BuildPictograph(const Graphs::Graph2D* oldGraph = nullptr);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnSelectVariables();
        void OnSelectShape();
        void OnDatasetChanged();
        void UpdateVariableLabels();
        void UpdateShapeLabel();
        static Data::Dataset::ColumnPreviewInfo
        BuildColumnPreviewInfo(const Data::Dataset& dataset);

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };
        constexpr static wxWindowID ID_SELECT_SHAPE_BUTTON{ wxID_HIGHEST + 5 };

        wxChoice* m_datasetChoice{ nullptr };
        wxStaticText* m_labelVarLabel{ nullptr };
        wxStaticText* m_valueVarLabel{ nullptr };
        wxStaticText* m_shapeLabel{ nullptr };

        // validators do not work with color pickers, so these are read directly
        wxColourPickerCtrl* m_fillColorPicker{ nullptr };
        wxColourPickerCtrl* m_outlineColorPicker{ nullptr };

        // DDX data members
        wxString m_labelVariable;
        wxString m_valueVariable;
        int m_orientationIndex{ 0 }; // 0 is vertical and 1 is horizontal
        int m_valueFormatIndex{ 0 }; // matches the order of the choices (see GetValueFormat())

        Icons::IconShape m_iconShape{ Icons::IconShape::Square };

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_PICTOGRAPH_DIALOG_H
