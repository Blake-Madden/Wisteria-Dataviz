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

#include "insertitemdlg.h"
#include <wx/choice.h>
#include <wx/wx.h>

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
            @param style The window style.*/
        InsertGraphDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                       const wxString& caption, wxWindowID id = wxID_ANY,
                       const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                       long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);

        /// @private
        InsertGraphDlg(const InsertGraphDlg&) = delete;
        /// @private
        InsertGraphDlg& operator=(const InsertGraphDlg&) = delete;

        /// @returns The selected legend placement.
        [[nodiscard]]
        LegendPlacement GetLegendPlacement() const;

      protected:
        /** @brief Creates a legend placement wxChoice and populates it.
            @param parent The parent window for the control.
            @param defaultSelection The initially selected item (0-based index).
            @returns The created wxChoice control.*/
        wxChoice* CreateLegendPlacementChoice(wxWindow* parent, int defaultSelection = 1);

        /// @brief Converts a legend wxChoice selection index to a LegendPlacement value.
        [[nodiscard]]
        static LegendPlacement SelectionToLegendPlacement(int selection);

      private:
        wxChoice* m_legendPlacementChoice{ nullptr };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_GRAPH_DIALOG_H
