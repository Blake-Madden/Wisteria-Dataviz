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
            if (title != m_titleExpanded)
                {
                graph.GetTitle().SetPropertyTemplate(L"text", title);
                }
            else if (!m_titleTemplate.empty())
                {
                graph.GetTitle().SetPropertyTemplate(L"text", m_titleTemplate);
                }
            }
        const auto subtitle = GetGraphSubtitle();
        if (!subtitle.empty())
            {
            graph.GetSubtitle().SetText(subtitle);
            if (subtitle != m_subtitleExpanded)
                {
                graph.GetSubtitle().SetPropertyTemplate(L"text", subtitle);
                }
            else if (!m_subtitleTemplate.empty())
                {
                graph.GetSubtitle().SetPropertyTemplate(L"text", m_subtitleTemplate);
                }
            }
        const auto caption = GetGraphCaption();
        if (!caption.empty())
            {
            graph.GetCaption().SetText(caption);
            if (caption != m_captionExpanded)
                {
                graph.GetCaption().SetPropertyTemplate(L"text", caption);
                }
            else if (!m_captionTemplate.empty())
                {
                graph.GetCaption().SetPropertyTemplate(L"text", m_captionTemplate);
                }
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
        m_legendPlacement = defaultSelection;
        auto* choice = new wxChoice(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr,
                                    0, wxGenericValidator(&m_legendPlacement));
        choice->Append(_(L"(None)"));
        choice->Append(_(L"Right"));
        choice->Append(_(L"Left"));
        choice->Append(_(L"Top"));
        choice->Append(_(L"Bottom"));
        return choice;
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
        return SelectionToLegendPlacement(m_legendPlacement);
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
        textSizer->Add(new wxTextCtrl(graphPage, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                      wxDefaultSize, 0, wxTextValidator(wxFILTER_NONE, &m_title)),
                       wxSizerFlags{}.Expand());

        textSizer->Add(new wxStaticText(graphPage, wxID_ANY, _(L"Subtitle:")),
                       wxSizerFlags{}.CenterVertical());
        textSizer->Add(new wxTextCtrl(graphPage, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                      wxDefaultSize, 0,
                                      wxTextValidator(wxFILTER_NONE, &m_subtitle)),
                       wxSizerFlags{}.Expand());

        textSizer->Add(new wxStaticText(graphPage, wxID_ANY, _(L"Caption:")),
                       wxSizerFlags{}.CenterVertical());
        textSizer->Add(new wxTextCtrl(graphPage, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                      wxDefaultSize, 0, wxTextValidator(wxFILTER_NONE, &m_caption)),
                       wxSizerFlags{}.Expand());

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
            {
            auto* opacitySpin = new wxSpinCtrl(graphPage, wxID_ANY);
            opacitySpin->SetRange(0, 255);
            opacitySpin->SetValue(255);
            opacitySpin->SetValidator(wxGenericValidator(&m_plotBgImageOpacity));
            opacitySizer->Add(opacitySpin);
            }
        graphSizer->Add(opacitySizer, wxSizerFlags{}.Border());

        // axis mirroring
        graphSizer->Add(new wxCheckBox(graphPage, wxID_ANY, _(L"Mirror X axis"), wxDefaultPosition,
                                       wxDefaultSize, 0, wxGenericValidator(&m_mirrorXAxis)),
                        wxSizerFlags{}.Border());

        graphSizer->Add(new wxCheckBox(graphPage, wxID_ANY, _(L"Mirror Y axis"), wxDefaultPosition,
                                       wxDefaultSize, 0, wxGenericValidator(&m_mirrorYAxis)),
                        wxSizerFlags{}.Border());
        }

    //-------------------------------------------
    wxString InsertGraphDlg::GetGraphTitle() const { return m_title; }

    //-------------------------------------------
    wxString InsertGraphDlg::GetGraphSubtitle() const { return m_subtitle; }

    //-------------------------------------------
    wxString InsertGraphDlg::GetGraphCaption() const { return m_caption; }

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
        return static_cast<uint8_t>(m_plotBgImageOpacity);
        }

    //-------------------------------------------
    bool InsertGraphDlg::GetMirrorXAxis() const { return m_mirrorXAxis; }

    //-------------------------------------------
    bool InsertGraphDlg::GetMirrorYAxis() const { return m_mirrorYAxis; }

    //-------------------------------------------
    void InsertGraphDlg::LoadGraphOptions(const Graphs::Graph2D& graph, const Canvas* canvas)
        {
        m_title = graph.GetTitle().GetText();
        m_subtitle = graph.GetSubtitle().GetText();
        m_caption = graph.GetCaption().GetText();

        // preserve raw templates for round-tripping {{placeholders}}
        m_titleTemplate = graph.GetTitle().GetPropertyTemplate(L"text");
        m_titleExpanded = m_title;
        m_subtitleTemplate = graph.GetSubtitle().GetPropertyTemplate(L"text");
        m_subtitleExpanded = m_subtitle;
        m_captionTemplate = graph.GetCaption().GetPropertyTemplate(L"text");
        m_captionExpanded = m_caption;

        const auto bgColor = graph.GetPlotBackgroundColor();
        if (bgColor.IsOk() && !bgColor.IsTransparent() && m_plotBgColorPicker != nullptr)
            {
            m_plotBgColorPicker->SetColour(bgColor);
            }

        m_mirrorXAxis = graph.IsXAxisMirrored();
        m_mirrorYAxis = graph.IsYAxisMirrored();

        // legend placement
        const auto& legendInfo = graph.GetLegendInfo();
        if (legendInfo.has_value())
            {
            switch (legendInfo->GetPlacement())
                {
            case Side::Right:
                m_legendPlacement = 1;
                break;
            case Side::Left:
                m_legendPlacement = 2;
                break;
            case Side::Top:
                m_legendPlacement = 3;
                break;
            case Side::Bottom:
                m_legendPlacement = 4;
                break;
            default:
                m_legendPlacement = 0;
                break;
                }
            }
        else
            {
            m_legendPlacement = 0;
            }

        LoadPageOptions(graph, canvas);
        }
    } // namespace Wisteria::UI
