/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_CAT_BARCHART_DIALOG_H
#define INSERT_CAT_BARCHART_DIALOG_H

#include "../../graphs/categoricalbarchart.h"
#include "insertgraphdlg.h"
#include <vector>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a categorical bar chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting a categorical column, an optional weight column,
              and an optional grouping column.
            - Labels showing the current variable selections.
            - A box effect choice.
            - Bar orientation (horizontal / vertical).
            - Bar label display.
            - A color scheme choice.
            - Legend placement.*/
    class InsertCatBarChartDlg final : public InsertGraphDlg
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
        InsertCatBarChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                             const wxString& caption = _(L"Insert Bar Chart"),
                             wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                             const wxSize& size = wxDefaultSize,
                             long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                          wxRESIZE_BORDER,
                             EditMode editMode = EditMode::Insert);

        /// @private
        InsertCatBarChartDlg(const InsertCatBarChartDlg&) = delete;
        /// @private
        InsertCatBarChartDlg& operator=(const InsertCatBarChartDlg&) = delete;

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

        /// @returns The categorical variable name.
        [[nodiscard]]
        const wxString& GetCategoricalVariable() const noexcept
            {
            return m_categoricalVariable;
            }

        /// @returns The weight (aggregate) variable name, or empty if using frequency.
        [[nodiscard]]
        const wxString& GetWeightVariable() const noexcept
            {
            return m_weightVariable;
            }

        /// @returns The grouping variable name, or empty if none.
        [[nodiscard]]
        const wxString& GetGroupVariable() const noexcept
            {
            return m_groupVariable;
            }

        /// @returns The selected box effect.
        [[nodiscard]]
        BoxEffect GetBoxEffect() const noexcept
            {
            return BoxEffectFromIndex(m_boxEffectIndex);
            }

        /// @returns The selected bar orientation.
        [[nodiscard]]
        Orientation GetBarOrientation() const noexcept
            {
            return (m_barOrientationIndex == 1) ? Orientation::Vertical : Orientation::Horizontal;
            }

        /// @returns The selected bar label display.
        [[nodiscard]]
        BinLabelDisplay GetBarLabelDisplay() const noexcept
            {
            return BinLabelDisplayFromIndex(m_barLabelDisplayIndex);
            }

        /// @returns The stipple shape (only meaningful when box effect is StippleShape).
        [[nodiscard]]
        Icons::IconShape GetStippleShape() const noexcept
            {
            return m_stippleShape;
            }

        /// @returns The stipple shape color
        ///     (only meaningful when box effect is StippleShape).
        [[nodiscard]]
        const wxColour& GetStippleShapeColor() const noexcept
            {
            return m_stippleShapeColor;
            }

        /// @returns The image file paths
        ///     (only meaningful when box effect is an image-based effect).
        [[nodiscard]]
        const wxArrayString& GetImagePaths() const noexcept
            {
            return m_imagePaths;
            }

        /// @returns Whether the user opted to override the default image size.
        [[nodiscard]]
        bool IsImageCustomSizeEnabled() const noexcept
            {
            return m_imageCustomSize;
            }

        /// @returns The requested image width.
        [[nodiscard]]
        int GetImageWidth() const noexcept
            {
            return m_imageWidth;
            }

        /// @returns The requested image height.
        [[nodiscard]]
        int GetImageHeight() const noexcept
            {
            return m_imageHeight;
            }

        /// @returns The selected image resize method.
        [[nodiscard]]
        ResizeMethod GetImageResizeMethod() const noexcept
            {
            return m_imageResizeMethod;
            }

        /// @returns The selected image effect.
        [[nodiscard]]
        ImageEffect GetImageEffect() const noexcept
            {
            return m_imageEffect;
            }

        /// @returns The image stitch direction.
        [[nodiscard]]
        Orientation GetImageStitchDirection() const noexcept
            {
            return m_imageStitchDirection;
            }

        /// @brief Populates all dialog controls from an existing categorical bar chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnSelectVariables();
        void OnDatasetChanged();
        void OnSelectStippleShape();
        void OnSelectImages();
        void OnBoxEffectChanged();
        void UpdateVariableLabels();
        Data::Dataset::ColumnPreviewInfo BuildColumnPreviewInfo(const Data::Dataset& dataset) const;

        [[nodiscard]]
        static BoxEffect BoxEffectFromIndex(int index) noexcept;
        [[nodiscard]]
        static int BoxEffectToIndex(BoxEffect effect) noexcept;
        [[nodiscard]]
        static BinLabelDisplay BinLabelDisplayFromIndex(int index) noexcept;
        [[nodiscard]]
        static int BinLabelDisplayToIndex(BinLabelDisplay display) noexcept;

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_DATASET_CHOICE{ wxID_HIGHEST + 3 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };

        wxChoice* m_datasetChoice{ nullptr };
        wxChoice* m_boxEffectChoice{ nullptr };
        wxStaticText* m_categoricalVarLabel{ nullptr };
        wxStaticText* m_weightVarLabel{ nullptr };
        wxStaticText* m_groupVarLabel{ nullptr };

        wxButton* m_shapeButton{ nullptr };
        wxStaticText* m_shapeLabel{ nullptr };
        wxButton* m_imagesButton{ nullptr };
        wxStaticText* m_imagesLabel{ nullptr };
        wxChoice* m_legendChoice{ nullptr };
        wxStaticText* m_legendLabel{ nullptr };

        // DDX data members
        int m_boxEffectIndex{ 0 };
        int m_barOrientationIndex{ 0 };
        int m_barLabelDisplayIndex{ 0 };

        wxString m_categoricalVariable;
        wxString m_weightVariable;
        wxString m_groupVariable;

        // stipple shape settings
        Icons::IconShape m_stippleShape{ Icons::IconShape::Square };
        wxColour m_stippleShapeColor{ *wxWHITE };

        // image settings for image-based box effects
        wxArrayString m_imagePaths;
        bool m_imageCustomSize{ false };
        int m_imageWidth{ 512 };
        int m_imageHeight{ 512 };
        ResizeMethod m_imageResizeMethod{ ResizeMethod::DownscaleOrUpscale };
        ImageEffect m_imageEffect{ ImageEffect::NoEffect };
        Orientation m_imageStitchDirection{ Orientation::Horizontal };

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_CAT_BARCHART_DIALOG_H
