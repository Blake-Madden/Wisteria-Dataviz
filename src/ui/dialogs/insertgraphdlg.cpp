///////////////////////////////////////////////////////////////////////////////
// Name:        insertgraphdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertgraphdlg.h"
#include "../../graphs/graph2d.h"

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertGraphDlg::InsertGraphDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                   wxWindow* parent, const wxString& caption, const wxWindowID id,
                                   const wxPoint& pos, const wxSize& size, const long style)
        : InsertItemDlg(canvas, reportBuilder, parent, caption, id, pos, size, style)
        {
        }

    //-------------------------------------------
    void InsertGraphDlg::ApplyGraphOptions(Graphs::Graph2D& graph) const
        {
        const auto title = GetGraphTitle();
        if (!title.empty())
            {
            graph.GetTitle().SetText(title);
            }
        const auto subtitle = GetGraphSubtitle();
        if (!subtitle.empty())
            {
            graph.GetSubtitle().SetText(subtitle);
            }
        const auto caption = GetGraphCaption();
        if (!caption.empty())
            {
            graph.GetCaption().SetText(caption);
            }

        const auto bgColor = GetPlotBackgroundColor();
        if (bgColor.IsOk() && bgColor != *wxWHITE)
            {
            graph.SetPlotBackgroundColor(bgColor);
            }

        const auto& bgImage = GetPlotBackgroundImage();
        if (bgImage.IsOk())
            {
            graph.SetPlotBackgroundImage(
                wxBitmapBundle::FromBitmap(wxBitmap(bgImage.GetOriginalImage())),
                GetPlotBackgroundImageOpacity());
            }

        graph.MirrorXAxis(GetMirrorXAxis());
        graph.MirrorYAxis(GetMirrorYAxis());
        }

    //-------------------------------------------
    wxChoice* InsertGraphDlg::CreateLegendPlacementChoice(wxWindow* parent,
                                                          const int defaultSelection)
        {
        m_legendPlacementChoice = new wxChoice(parent, wxID_ANY);
        m_legendPlacementChoice->Append(_(L"(None)"));
        m_legendPlacementChoice->Append(_(L"Right"));
        m_legendPlacementChoice->Append(_(L"Left"));
        m_legendPlacementChoice->Append(_(L"Top"));
        m_legendPlacementChoice->Append(_(L"Bottom"));
        m_legendPlacementChoice->SetSelection(defaultSelection);
        return m_legendPlacementChoice;
        }

    //-------------------------------------------
    LegendPlacement InsertGraphDlg::SelectionToLegendPlacement(const int selection)
        {
        switch (selection)
            {
        case 1:
            return LegendPlacement::Right;
        case 2:
            return LegendPlacement::Left;
        case 3:
            return LegendPlacement::Top;
        case 4:
            return LegendPlacement::Bottom;
        default:
            return LegendPlacement::None;
            }
        }

    //-------------------------------------------
    LegendPlacement InsertGraphDlg::GetLegendPlacement() const
        {
        if (m_legendPlacementChoice == nullptr)
            {
            return LegendPlacement::None;
            }
        return SelectionToLegendPlacement(m_legendPlacementChoice->GetSelection());
        }

    //-------------------------------------------
    void InsertGraphDlg::CreateGraphOptionsPage()
        {
        auto* graphPage = new wxPanel(GetSideBarBook());
        auto* graphSizer = new wxBoxSizer(wxVERTICAL);
        graphPage->SetSizer(graphSizer);
        GetSideBarBook()->AddPage(graphPage, _(L"Graph Options"), ID_GRAPH_OPTIONS_SECTION, false);

        // title, subtitle, caption
        auto* textSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        textSizer->AddGrowableCol(1, 1);

        textSizer->Add(new wxStaticText(graphPage, wxID_ANY, _(L"Title:")),
                       wxSizerFlags{}.CenterVertical());
        m_titleCtrl = new wxTextCtrl(graphPage, wxID_ANY);
        textSizer->Add(m_titleCtrl, wxSizerFlags{}.Expand());

        textSizer->Add(new wxStaticText(graphPage, wxID_ANY, _(L"Subtitle:")),
                       wxSizerFlags{}.CenterVertical());
        m_subtitleCtrl = new wxTextCtrl(graphPage, wxID_ANY);
        textSizer->Add(m_subtitleCtrl, wxSizerFlags{}.Expand());

        textSizer->Add(new wxStaticText(graphPage, wxID_ANY, _(L"Caption:")),
                       wxSizerFlags{}.CenterVertical());
        m_captionCtrl = new wxTextCtrl(graphPage, wxID_ANY);
        textSizer->Add(m_captionCtrl, wxSizerFlags{}.Expand());

        graphSizer->Add(textSizer, wxSizerFlags{}.Expand().Border());

        // background color
        auto* bgSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        bgSizer->Add(new wxStaticText(graphPage, wxID_ANY, _(L"Background color:")),
                     wxSizerFlags{}.CenterVertical());
        m_plotBgColorPicker = new wxColourPickerCtrl(graphPage, wxID_ANY, *wxWHITE);
        bgSizer->Add(m_plotBgColorPicker);

        graphSizer->Add(bgSizer, wxSizerFlags{}.Expand().Border());

        // background image
        graphSizer->Add(new wxStaticText(graphPage, wxID_ANY, _(L"Background image:")),
                        wxSizerFlags{}.Border(wxLEFT));
        m_plotBgImageThumbnail = new Thumbnail(
            graphPage, wxNullBitmap, ClickMode::BrowseForImageFile, true, wxID_ANY,
            wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE | wxBORDER_SIMPLE);
        graphSizer->Add(m_plotBgImageThumbnail, wxSizerFlags{}.Border());

        // image opacity
        auto* opacitySizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        opacitySizer->Add(new wxStaticText(graphPage, wxID_ANY, _(L"Image opacity:")),
                          wxSizerFlags{}.CenterVertical());
        m_plotBgImageOpacitySpin = new wxSpinCtrl(graphPage, wxID_ANY);
        m_plotBgImageOpacitySpin->SetRange(0, 255);
        m_plotBgImageOpacitySpin->SetValue(255);
        opacitySizer->Add(m_plotBgImageOpacitySpin);
        graphSizer->Add(opacitySizer, wxSizerFlags{}.Border());

        // axis mirroring
        m_mirrorXAxisCheck = new wxCheckBox(graphPage, wxID_ANY, _(L"Mirror X axis"));
        m_mirrorXAxisCheck->SetValue(false);
        graphSizer->Add(m_mirrorXAxisCheck, wxSizerFlags{}.Border());

        m_mirrorYAxisCheck = new wxCheckBox(graphPage, wxID_ANY, _(L"Mirror Y axis"));
        m_mirrorYAxisCheck->SetValue(false);
        graphSizer->Add(m_mirrorYAxisCheck, wxSizerFlags{}.Border());
        }

    //-------------------------------------------
    wxString InsertGraphDlg::GetGraphTitle() const
        {
        return (m_titleCtrl != nullptr) ? m_titleCtrl->GetValue() : wxString{};
        }

    //-------------------------------------------
    wxString InsertGraphDlg::GetGraphSubtitle() const
        {
        return (m_subtitleCtrl != nullptr) ? m_subtitleCtrl->GetValue() : wxString{};
        }

    //-------------------------------------------
    wxString InsertGraphDlg::GetGraphCaption() const
        {
        return (m_captionCtrl != nullptr) ? m_captionCtrl->GetValue() : wxString{};
        }

    //-------------------------------------------
    wxColour InsertGraphDlg::GetPlotBackgroundColor() const
        {
        return (m_plotBgColorPicker != nullptr) ? m_plotBgColorPicker->GetColour() : wxColour{};
        }

    //-------------------------------------------
    const GraphItems::Image& InsertGraphDlg::GetPlotBackgroundImage() const
        {
        return m_plotBgImageThumbnail->GetImage();
        }

    //-------------------------------------------
    uint8_t InsertGraphDlg::GetPlotBackgroundImageOpacity() const
        {
        return (m_plotBgImageOpacitySpin != nullptr) ?
                   static_cast<uint8_t>(m_plotBgImageOpacitySpin->GetValue()) :
                   wxALPHA_OPAQUE;
        }

    //-------------------------------------------
    bool InsertGraphDlg::GetMirrorXAxis() const
        {
        return (m_mirrorXAxisCheck != nullptr) && m_mirrorXAxisCheck->GetValue();
        }

    //-------------------------------------------
    bool InsertGraphDlg::GetMirrorYAxis() const
        {
        return (m_mirrorYAxisCheck != nullptr) && m_mirrorYAxisCheck->GetValue();
        }
    } // namespace Wisteria::UI
