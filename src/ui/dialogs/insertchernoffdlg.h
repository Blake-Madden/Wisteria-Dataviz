/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_CHERNOFF_DIALOG_H
#define INSERT_CHERNOFF_DIALOG_H

#include "../../graphs/chernoffplot.h"
#include "insertgraphdlg.h"
#include <array>
#include <vector>
#include <wx/clrpicker.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a Chernoff Faces plot into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg.
            - A label grid showing the feature-to-variable mappings.
            - Choice boxes for gender, hair style, facial hair, and color pickers.*/
    class InsertChernoffDlg final : public InsertGraphDlg
        {
      public:
        /** @brief Constructor.
            @param canvas The canvas whose grid layout is displayed.
            @param reportBuilder The report builder containing the project's datasets.
            @param parent The parent window.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.*/
        InsertChernoffDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                          wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize,
                          long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);

        /// @private
        InsertChernoffDlg(const InsertChernoffDlg&) = delete;
        /// @private
        InsertChernoffDlg& operator=(const InsertChernoffDlg&) = delete;

        /// @returns The selected dataset, or @c nullptr if none.
        [[nodiscard]]
        std::shared_ptr<Data::Dataset> GetSelectedDataset() const;

        /// @returns The selected gender.
        [[nodiscard]]
        Gender GetGender() const;

        /// @returns The selected hair style.
        [[nodiscard]]
        HairStyle GetHairStyle() const;

        /// @returns The selected facial hair style.
        [[nodiscard]]
        FacialHair GetFacialHair() const;

        /// @returns The lighter skin color (low saturation).
        [[nodiscard]]
        wxColour GetSkinColorLighter() const
            {
            return m_skinColorLighterPicker->GetColour();
            }

        /// @returns The darker skin color (high saturation).
        [[nodiscard]]
        wxColour GetSkinColorDarker() const
            {
            return m_skinColorDarkerPicker->GetColour();
            }

        /// @returns The selected eye color.
        [[nodiscard]]
        wxColour GetEyeColor() const
            {
            return m_eyeColorPicker->GetColour();
            }

        /// @returns The selected hair color.
        [[nodiscard]]
        wxColour GetHairColor() const
            {
            return m_hairColorPicker->GetColour();
            }

        /// @returns Whether to show labels beneath faces.
        [[nodiscard]]
        bool GetShowLabels() const
            {
            return m_showLabelsCheck->GetValue();
            }

        /// @returns The variable name for a given feature, or empty if unassigned.
        /// @param feature The facial feature to look up.
        [[nodiscard]]
        wxString GetFeatureVariable(Graphs::ChernoffFacesPlot::FeatureId feature) const;

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnSelectVariables();
        void OnDatasetChanged();
        void UpdateFeatureLabels();
        Data::Dataset::ColumnPreviewInfo BuildColumnPreviewInfo(const Data::Dataset& dataset) const;

        /// @brief The number of facial features available for variable mapping.
        constexpr static size_t FEATURE_COUNT{ 11 };

        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_DATASET_CHOICE{ wxID_HIGHEST + 3 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };

        wxChoice* m_datasetChoice{ nullptr };
        std::array<wxStaticText*, FEATURE_COUNT> m_featureVarLabels{};
        wxChoice* m_genderChoice{ nullptr };
        wxChoice* m_hairStyleChoice{ nullptr };
        wxStaticText* m_facialHairLabel{ nullptr };
        wxChoice* m_facialHairChoice{ nullptr };
        wxColourPickerCtrl* m_skinColorLighterPicker{ nullptr };
        wxColourPickerCtrl* m_skinColorDarkerPicker{ nullptr };
        wxColourPickerCtrl* m_eyeColorPicker{ nullptr };
        wxColourPickerCtrl* m_hairColorPicker{ nullptr };
        wxCheckBox* m_showLabelsCheck{ nullptr };

        // feature-to-variable mappings (indexed by FeatureId)
        std::map<Graphs::ChernoffFacesPlot::FeatureId, wxString> m_featureVariables;

        // cached dataset names (parallel to choice box indices)
        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_CHERNOFF_DIALOG_H
