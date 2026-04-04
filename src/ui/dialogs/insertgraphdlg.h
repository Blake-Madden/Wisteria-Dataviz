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

#include "../../base/axis.h"
#include "../../base/colorbrewer.h"
#include "../../base/image.h"
#include "../../base/label.h"
#include "insertitemdlg.h"
#include <wx/choice.h>
#include <wx/clrpicker.h>
#include <wx/editlbox.h>
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

    /// @brief Flags controlling which sections are visible in InsertGraphDlg.
    enum GraphDlgOptions : int
        {
        GraphDlgIncludeColorScheme = 1 << 0, ///< Show the color scheme controls.
        /// @brief All options enabled (the default).
        GraphDlgIncludeAll = GraphDlgIncludeColorScheme
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
                       EditMode editMode = EditMode::Insert,
                       GraphDlgOptions options = GraphDlgIncludeAll);

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

        /** @brief Restores axis state from the original graph.
            @details Call this after SetData() since SetData() rebuilds axes,
                discarding titles, brackets, pens, custom labels, etc.
            @param graph The graph to apply the saved axis state to.*/
        void ApplyAxisOverrides(Graphs::Graph2D& graph) const;

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

        /// @returns @c true if the user chose a custom color list
        ///     instead of a named color scheme.
        [[nodiscard]]
        bool IsUsingCustomColors() const noexcept
            {
            return m_useCustomColors;
            }

        /// @returns The selected named color scheme, or @c nullptr for default.
        /// @note Only meaningful when IsUsingCustomColors() is @c false.
        [[nodiscard]]
        std::shared_ptr<Colors::Schemes::ColorScheme> GetColorScheme() const
            {
            return ColorSchemeFromIndex(m_colorSchemeIndex);
            }

        /// @returns The user-defined custom color list.
        /// @note Only meaningful when IsUsingCustomColors() is @c true.
        [[nodiscard]]
        const std::vector<wxColour>& GetCustomColors() const noexcept
            {
            return m_customColors;
            }

        /// @brief Populates the graph options controls from an existing Graph2D.
        /// @param graph The graph to read options from.
        void LoadGraphOptions(const Graphs::Graph2D& graph);

        /// @brief Maps an existing color scheme to a dropdown index.
        /// @param scheme The color scheme to identify (may be @c nullptr).
        /// @returns The corresponding index (0 if unrecognized or @c nullptr).
        [[nodiscard]]
        static int ColorSchemeToIndex(const std::shared_ptr<Colors::Schemes::ColorScheme>& scheme);

        /// @brief Returns the JSON key name for a color scheme dropdown index.
        /// @param index The zero-based selection index (0 = default/none).
        /// @returns The JSON key string, or empty if index is 0 or out of range.
        [[nodiscard]]
        static wxString ColorSchemeToName(int index);

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

        /// @brief Validates the color scheme selection.
        /// @returns @c true if valid, @c false if the user needs to fix something.
        [[nodiscard]]
        bool ValidateColorScheme();

        /** @brief Creates and adds the "Graph Options" sidebar page.
            @details This page contains controls common to all Graph2D types:
                title, subtitle, caption, background color, and background image.
            @note Call this from derived CreateControls() after the base
                InsertGraphDlg::CreateControls() and before adding
                chart-specific pages.*/
        void CreateGraphOptionsPage();

        /** @brief Creates and adds the "Axis Options" sidebar page.
            @details This page contains axis-related options such as
                mirroring the X and Y axes.
            @note Call this from derived CreateControls() for graph types
                that display axes. Omit for graphs that hide all axes
                (e.g., heat maps, waffle charts, word clouds).*/
        void CreateAxisOptionsPage();

        /// @brief Creates a color scheme from a dropdown index.
        /// @param index The zero-based selection index (0 = default/none).
        /// @returns The corresponding color scheme, or @c nullptr for "Default".
        [[nodiscard]]
        static std::shared_ptr<Colors::Schemes::ColorScheme> ColorSchemeFromIndex(int index);

        /// @brief Returns the list of color scheme display names for a @c wxChoice.
        /// @returns The array of color scheme names.
        [[nodiscard]]
        static wxArrayString GetColorSchemeNames();

        /// @brief ID for the Graph Options sidebar section.
        constexpr static wxWindowID ID_GRAPH_OPTIONS_SECTION{ wxID_HIGHEST + 100 };
        /// @brief ID for the Axis Options sidebar section.
        constexpr static wxWindowID ID_AXIS_OPTIONS_SECTION{ wxID_HIGHEST + 101 };

      private:
        void OnEditTitle();
        void OnEditSubtitle();
        void OnEditCaption();
        void EditLabelHelper(GraphItems::Label& label, wxStaticText* preview,
                             const wxString& caption);
        void OnEditBackgroundImage();
        void OnAddCustomColor();
        void OnEditCustomColor();
        void OnRemoveCustomColor();
        void RefreshCustomColorList();
        void OnColorModeChanged();

        GraphDlgOptions m_options{ GraphDlgIncludeAll };

        // DDX data members
        int m_legendPlacement{ 1 };
        bool m_mirrorXAxis{ false };
        bool m_mirrorYAxis{ false };
        int m_plotBgImageOpacity{ 255 };
        int m_plotBgImageFit{ 1 }; // Shrink
        bool m_useCustomColors{ false };
        int m_colorSchemeIndex{ 0 };
        std::vector<wxColour> m_customColors;

        // title / subtitle / caption as full Labels
        GraphItems::Label m_titleLabel;
        GraphItems::Label m_subtitleLabel;
        GraphItems::Label m_captionLabel;

        // background image
        GraphItems::Image m_plotBgImage;

        /// @brief Saved axes for round-tripping through the edit flow.
        /// @details SetData() rebuilds axes from scratch, so the full axis
        ///     state (title, pens, brackets, custom labels, etc.) must be
        ///     captured before the edit and restored afterward.
        std::map<AxisType, GraphItems::Axis> m_savedAxes;

        // preview controls
        wxStaticText* m_titlePreview{ nullptr };
        wxStaticText* m_subtitlePreview{ nullptr };
        wxStaticText* m_captionPreview{ nullptr };
        wxStaticText* m_bgImagePreview{ nullptr };

        // controls without DDX validator support
        wxColourPickerCtrl* m_plotBgColorPicker{ nullptr };
        wxRadioButton* m_namedSchemeRadio{ nullptr };
        wxRadioButton* m_customColorsRadio{ nullptr };
        wxChoice* m_colorSchemeChoice{ nullptr };
        wxEditableListBox* m_customColorListBox{ nullptr };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_GRAPH_DIALOG_H
