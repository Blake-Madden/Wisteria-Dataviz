/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_GRAPH_DIALOG_H
#define INSERT_GRAPH_DIALOG_H

#include "../../base/image.h"
#include "../../base/label.h"
#include "insertitemdlg.h"
#include <wx/choice.h>
#include <wx/clrpicker.h>
#include <wx/spinctrl.h>
#include <wx/valgen.h>
#include <wx/valtext.h>
#include <wx/wx.h>

namespace Wisteria::Graphs
    {
    class Graph2D;
    }

namespace Wisteria::UI
    {
    /// @brief Legend placement options for insert dialogs.
    enum class LegendPlacement
        {
        None,  /*!< No legend.*/
        Right, /*!< Legend to the right of the graph.*/
        Left,  /*!< Legend to the left of the graph.*/
        Top,   /*!< Legend above the graph.*/
        Bottom /*!< Legend below the graph.*/
        };

    /** @brief Intermediate base dialog for inserting a graph into a canvas cell.
        @details Extends InsertItemDlg with common graph options such as
            legend placement. Derived dialogs for specific chart types
            should inherit from this class rather than InsertItemDlg directly.*/
    class InsertGraphDlg : public InsertItemDlg
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
        InsertGraphDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                       const wxString& caption, wxWindowID id = wxID_ANY,
                       const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                       long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                       EditMode editMode = EditMode::Insert);

        /// @private
        InsertGraphDlg(const InsertGraphDlg&) = delete;
        /// @private
        InsertGraphDlg& operator=(const InsertGraphDlg&) = delete;

        /** @brief Applies the common graph options to a Graph2D object.
            @details Sets the title, subtitle, caption, background color,
                background image (with opacity), and axis mirroring based on
                the dialog's current control values.
            @param graph The graph to apply the options to.*/
        void ApplyGraphOptions(Graphs::Graph2D& graph) const;

        /// @returns The selected legend placement.
        [[nodiscard]]
        LegendPlacement GetLegendPlacement() const;

        /// @returns The graph title label.
        [[nodiscard]]
        const GraphItems::Label& GetTitleLabel() const noexcept
            {
            return m_titleLabel;
            }

        /// @returns The graph subtitle label.
        [[nodiscard]]
        const GraphItems::Label& GetSubtitleLabel() const noexcept
            {
            return m_subtitleLabel;
            }

        /// @returns The graph caption label.
        [[nodiscard]]
        const GraphItems::Label& GetCaptionLabel() const noexcept
            {
            return m_captionLabel;
            }

        /// @returns The selected plot background color.
        ///     May be invalid if the user did not change it.
        [[nodiscard]]
        wxColour GetPlotBackgroundColor() const;

        /// @returns The background image settings.
        [[nodiscard]]
        const GraphItems::Image& GetPlotBackgroundImage() const noexcept
            {
            return m_plotBgImage;
            }

        /// @returns The opacity for the plot background image (0-255).
        [[nodiscard]]
        uint8_t GetPlotBackgroundImageOpacity() const noexcept
            {
            return static_cast<uint8_t>(m_plotBgImageOpacity);
            }

        /// @returns The plot background image fit mode.
        [[nodiscard]]
        ImageFit GetPlotBackgroundImageFit() const noexcept
            {
            return static_cast<ImageFit>(m_plotBgImageFit);
            }

        /// @returns Whether to mirror the X axis.
        [[nodiscard]]
        bool GetMirrorXAxis() const;

        /// @returns Whether to mirror the Y axis.
        [[nodiscard]]
        bool GetMirrorYAxis() const;

        /// @brief Populates the graph options controls from an existing Graph2D.
        /// @param graph The graph to read options from.
        /// @param canvas The canvas the graph belongs to.
        void LoadGraphOptions(const Graphs::Graph2D& graph, const Canvas* canvas);

      protected:
        /// @returns The color to apply to selected variable names on the dialog.
        static wxColour GetVariableLabelColor() { return wxColour{ 0, 102, 204 }; }

        /** @brief Creates a legend placement wxChoice and populates it.
            @param parent The parent window for the control.
            @param defaultSelection The initially selected item (0-based index).
            @returns The created wxChoice control.*/
        wxChoice* CreateLegendPlacementChoice(wxWindow* parent, int defaultSelection = 1);

        /// @brief Converts a legend wxChoice selection index to a LegendPlacement value.
        /// @param selection The zero-based index from the wxChoice control.
        /// @returns The corresponding LegendPlacement value.
        [[nodiscard]]
        static LegendPlacement SelectionToLegendPlacement(int selection);

        /** @brief Creates and adds the "Graph Options" sidebar page.
            @details This page contains controls common to all Graph2D types:
                title, subtitle, caption, background color, background image,
                and axis mirroring options.
            @note Call this from derived CreateControls() after the base
                InsertGraphDlg::CreateControls() and before adding
                chart-specific pages.*/
        void CreateGraphOptionsPage();

        /// @brief ID for the Graph Options sidebar section.
        constexpr static wxWindowID ID_GRAPH_OPTIONS_SECTION{ wxID_HIGHEST + 100 };

      private:
        void OnEditTitle();
        void OnEditSubtitle();
        void OnEditCaption();
        void EditLabelHelper(GraphItems::Label& label, wxStaticText* preview,
                             const wxString& caption);
        void OnEditBackgroundImage();

        // DDX data members
        int m_legendPlacement{ 1 };
        bool m_mirrorXAxis{ false };
        bool m_mirrorYAxis{ false };
        int m_plotBgImageOpacity{ 255 };
        int m_plotBgImageFit{ 1 }; // Shrink

        // title / subtitle / caption as full Labels
        GraphItems::Label m_titleLabel;
        GraphItems::Label m_subtitleLabel;
        GraphItems::Label m_captionLabel;

        // background image
        GraphItems::Image m_plotBgImage;

        // preview controls
        wxStaticText* m_titlePreview{ nullptr };
        wxStaticText* m_subtitlePreview{ nullptr };
        wxStaticText* m_captionPreview{ nullptr };
        wxStaticText* m_bgImagePreview{ nullptr };

        // controls without DDX validator support
        wxColourPickerCtrl* m_plotBgColorPicker{ nullptr };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_GRAPH_DIALOG_H
