/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_CHOROPLETH_MAP_DIALOG_H
#define INSERT_CHOROPLETH_MAP_DIALOG_H

#include "insertgraphdlg.h"
#include <vector>
#include <wx/clrpicker.h>
#include <wx/filepicker.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a choropleth map into a canvas.
        @details Extends InsertGraphDlg with a "Choropleth Map" page containing:
            - A picker for the KML file that supplies the region shapes.
            - An optional @c ExtendedData field name to use as the region key.
            - An optional project dataset, plus a variable selector for the key
              column (matched against the region key) and either a value column
              (a color gradient) or a category column (a color per category).
            - A "Show region labels" toggle.*/
    class InsertChoroplethMapDlg final : public InsertGraphDlg
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
        InsertChoroplethMapDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                               const wxString& caption = _(L"Insert Choropleth Map"),
                               wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                               const wxSize& size = wxDefaultSize,
                               long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                            wxRESIZE_BORDER,
                               EditMode editMode = EditMode::Insert);

        /// @private
        InsertChoroplethMapDlg(const InsertChoroplethMapDlg&) = delete;
        /// @private
        InsertChoroplethMapDlg& operator=(const InsertChoroplethMapDlg&) = delete;

        /// @returns The path to the selected KML file.
        [[nodiscard]]
        wxString GetKMLPath() const;

        /// @returns The @c ExtendedData field to use as the region key,
        ///     or empty to use each placemark's name.
        [[nodiscard]]
        const wxString& GetKMLIdField() const noexcept
            {
            return m_kmlIdField;
            }

        /// @returns The selected dataset to map onto the regions, or @c nullptr if none.
        [[nodiscard]]
        std::shared_ptr<Data::Dataset> GetSelectedDataset() const;

        /// @returns The name of the selected dataset, or empty if none.
        [[nodiscard]]
        wxString GetSelectedDatasetName() const;

        /// @returns The dataset column matched against the region key, or empty if none.
        [[nodiscard]]
        wxString GetKeyColumn() const;

        /// @returns The continuous column used to shade the regions, or empty if none.
        [[nodiscard]]
        wxString GetValueColumn() const;

        /// @returns The categorical column used to shade the regions, or empty if none.
        [[nodiscard]]
        wxString GetCategoryColumn() const;

        /// @returns The continuous column used to size the proportional shape,
        ///     or empty if none.
        [[nodiscard]]
        wxString GetSymbolColumn() const;

        /// @returns @c true if a proportional shape size column was chosen.
        [[nodiscard]]
        bool IsUsingProportionalSymbols() const;

        /// @returns The classification method for the value column, as a
        ///     ChoroplethMap::ClassificationMethod cast to @c int (0 is unclassed).
        [[nodiscard]]
        int GetClassificationMethod() const noexcept
            {
            return m_classificationMethod;
            }

        /// @returns The number of classes to split the value column into.
        [[nodiscard]]
        int GetClassCount() const noexcept
            {
            return m_classCount;
            }

        /// @returns The fill color chosen for the proportional shapes.
        [[nodiscard]]
        const wxColour& GetProportionalSymbolColor() const noexcept
            {
            return m_symbolColor;
            }

        /// @returns @c true if a dataset and a value or category column were chosen.
        [[nodiscard]]
        bool IsMappingData() const;

        /// @returns @c true if region labels should be drawn.
        [[nodiscard]]
        bool IsShowingRegionLabels() const;

        /// @returns @c true if the latitude/longitude graticule should be drawn.
        [[nodiscard]]
        bool IsShowingGraticule() const noexcept
            {
            return m_showGraticule;
            }

        /// @returns What a region's label shows.
        [[nodiscard]]
        int GetRegionLabelDisplay() const noexcept
            {
            return m_labelDisplay;
            }

        /// @returns The fill style chosen for regions with no mapped value.
        [[nodiscard]]
        wxBrushStyle GetNoDataFillStyle() const noexcept
            {
            switch (m_noDataFillStyle)
                {
            case 1:
                return wxBRUSHSTYLE_FDIAGONAL_HATCH;
            case 2:
                return wxBRUSHSTYLE_BDIAGONAL_HATCH;
            case 3:
                return wxBRUSHSTYLE_CROSSDIAG_HATCH;
            case 4:
                return wxBRUSHSTYLE_CROSS_HATCH;
            case 5:
                return wxBRUSHSTYLE_HORIZONTAL_HATCH;
            case 6:
                return wxBRUSHSTYLE_VERTICAL_HATCH;
            default:
                return wxBRUSHSTYLE_SOLID;
                }
            }

        /// @brief Populates all dialog controls from an existing choropleth map.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnDatasetChanged();

        void OnSelectVariables();

        void UpdateVariableLabels();

        void UpdateClassificationControls();

        /// @brief Builds column preview info (name + type) for a project dataset,
        ///     so it can drive a VariableSelectDlg.
        /// @param dataset The dataset to describe.
        /// @returns The preview info, ID column first, then continuous and categorical.
        [[nodiscard]]
        Data::Dataset::ColumnPreviewInfo BuildColumnPreviewInfo(const Data::Dataset& dataset) const;

        void OnKMLFileChanged();

        /// @brief Fills the region-key-field dropdown with the attribute fields
        ///     declared in @p kmlPath, keeping whatever the user has typed.
        void PopulateKeyFieldChoices(const wxString& kmlPath);

        /// @brief Maps a no-data fill style to its index in the dialog's
        ///     "Regions with no data" fill choice.
        /// @param style The brush style.
        /// @returns The choice index, @c 0 (solid) for an unrecognized style.
        [[nodiscard]]
        static int NoDataFillStyleToChoiceIndex(wxBrushStyle style);

        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_DATASET_CHOICE{ wxID_HIGHEST + 3 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };

        wxFilePickerCtrl* m_kmlPicker{ nullptr };
        wxComboBox* m_kmlIdFieldCombo{ nullptr };
        wxChoice* m_datasetChoice{ nullptr };
        wxButton* m_selectVarsButton{ nullptr };
        wxStaticText* m_keyColumnLabel{ nullptr };
        wxStaticText* m_valueColumnLabel{ nullptr };
        wxStaticText* m_categoryColumnLabel{ nullptr };
        wxStaticText* m_symbolColumnLabel{ nullptr };
        wxStaticText* m_symbolColorLabel{ nullptr };
        wxColourPickerCtrl* m_symbolColorPicker{ nullptr };
        wxChoice* m_classificationChoice{ nullptr };
        wxStaticText* m_classCountLabel{ nullptr };
        wxSpinCtrl* m_classCountSpin{ nullptr };

        wxString m_kmlIdField;
        bool m_showLabels{ false };
        bool m_showGraticule{ false };
        // 4 is BinLabelDisplay::BinName.
        int m_labelDisplay{ 4 };
        // 0 is a solid fill.
        int m_noDataFillStyle{ 0 };
        // 0 is unclassed
        int m_classificationMethod{ 0 };
        int m_classCount{ 5 };

        wxString m_keyColumn;
        wxString m_valueColumn;
        wxString m_categoryColumn;
        wxString m_symbolColumn;
        wxColour m_symbolColor{ wxColour(L"#2F6F8F") };

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_CHOROPLETH_MAP_DIALOG_H
