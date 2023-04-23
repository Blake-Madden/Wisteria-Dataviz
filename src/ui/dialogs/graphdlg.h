/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __GRAPH_DLG_H__
#define __GRAPH_DLG_H__

#include <wx/wx.h>
#include <wx/string.h>
#include <wx/statline.h>
#include <wx/artprov.h>
#include <vector>
#include "../../base/canvas.h"

namespace Wisteria::UI
    {
    /** @brief Dialog for showing a graph.
        @details Includes buttons for saving, copying, and printing the graph.*/
    class GraphDlg final : public wxDialog
        {
    public:
        /** @brief Constructor.
            @param parent the parent of the dialog.
            @param id the window ID for this dialog.
            @param caption the title of this dialog.
            @note This dialog takes ownership of the graph and will delete it upon destruction.
                Therefore, the graph should be created on the heap (with @c new).*/
        GraphDlg(wxWindow* parent, wxWindowID id = wxID_ANY,
                 const wxString& caption = _(L"View Graph"))
            {
            Create(parent, id, caption, wxDefaultPosition, wxDefaultSize,
                   wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN|wxRESIZE_BORDER);
            }
        /// @private
        GraphDlg() = delete;
        /// @private
        GraphDlg(const GraphDlg&) = delete;
        /// @private
        GraphDlg(GraphDlg&&) = delete;
        /// @private
        GraphDlg& operator==(const GraphDlg&) = delete;
        /// @private
        GraphDlg& operator==(GraphDlg&&) = delete;

        /// @returns Access to the canvas, where you can add the graph
        ///     (it has already been initialized to hold one graph).
        [[nodiscard]] Wisteria::Canvas* GetCanvas() noexcept
            { return m_canvas; }
    private:
        /// Creation step.
        bool Create(wxWindow* parent, wxWindowID id = wxID_ANY,
                    const wxString& caption = _(L"View Graph"),
                    const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                    long style = wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN|wxRESIZE_BORDER)
            {
            SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
            wxDialog::Create(parent, id, caption, pos, size, style);
            SetMinSize(FromDIP(wxSize(800, 600)));

            CreateControls();

            Bind(wxEVT_COMMAND_BUTTON_CLICKED, &GraphDlg::OnButtonClick, this, wxID_CLOSE);
            Bind(wxEVT_COMMAND_BUTTON_CLICKED, &GraphDlg::OnButtonClick, this, wxID_PRINT);
            Bind(wxEVT_COMMAND_BUTTON_CLICKED, &GraphDlg::OnButtonClick, this, wxID_SAVE);
            Bind(wxEVT_COMMAND_BUTTON_CLICKED, &GraphDlg::OnButtonClick, this, wxID_COPY);

            Centre();
            return true;
            }

        /// Create the controls.
        void CreateControls()
            {
            wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
            mainSizer->SetMinSize(FromDIP(wxSize(800, 600)));

            m_canvas = new Wisteria::Canvas(this);
            m_canvas->SetFixedObjectsGridSize(1, 1);
            mainSizer->Add(m_canvas, 1, wxALL|wxEXPAND, wxSizerFlags::GetDefaultBorder());

            mainSizer->Add(new wxStaticLine(this), wxSizerFlags().Expand().
                Border(wxRIGHT|wxLEFT,wxSizerFlags::GetDefaultBorder()));
            wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
                {
                wxButton* button = new wxButton(this, wxID_PRINT);
                button->SetBitmap(wxArtProvider::GetBitmapBundle(wxART_PRINT, wxART_BUTTON));
                buttonSizer->Add(button);
                }
            buttonSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
                {
                wxButton* button = new wxButton(this, wxID_COPY);
                button->SetBitmap(wxArtProvider::GetBitmapBundle(wxART_COPY, wxART_BUTTON));
                buttonSizer->Add(button);
                }
            buttonSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
                {
                wxButton* button = new wxButton(this, wxID_SAVE);
                button->SetBitmap(wxArtProvider::GetBitmapBundle(wxART_FILE_SAVE, wxART_BUTTON));
                buttonSizer->Add(button);
                }
            buttonSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
                {
                wxButton* button = new wxButton(this, wxID_CLOSE);
                button->SetBitmap(wxArtProvider::GetBitmapBundle(wxART_CLOSE, wxART_BUTTON));
                button->SetDefault();
                buttonSizer->Add(button);
                }
            mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT|wxALL, wxSizerFlags::GetDefaultBorder());

            SetSizerAndFit(mainSizer);
            }

        // Button clicking
        void OnButtonClick(wxCommandEvent& event)
            {
            if (event.GetId() == wxID_CLOSE)
                { Close(); }
            else if (event.GetId() == wxID_PRINT)
                { m_canvas->OnPrint(event); }
            else if (event.GetId() == wxID_SAVE)
                { m_canvas->OnSave(event); }
            else if (event.GetId() == wxID_COPY)
                { m_canvas->OnCopy(event); }
            else
                { event.Skip(); }
            }
        Wisteria::Canvas* m_canvas{ nullptr };
        };
    }

/** @}*/

#endif //__GRAPH_DLG_H__
