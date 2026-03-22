///////////////////////////////////////////////////////////////////////////////
// Name:        insertgraphdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertgraphdlg.h"
#include "../../base/reportenumconvert.h"
#include "../../graphs/graph2d.h"
#include "insertimgdlg.h"
#include "insertlabeldlg.h"
#include <wx/filename.h>
#include <wx/gbsizer.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertGraphDlg::InsertGraphDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                   wxWindow* parent, const wxString& caption, const wxWindowID id,
                                   const wxPoint& pos, const wxSize& size, const long style,
                                   EditMode editMode)
        : InsertItemDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        }

    //-------------------------------------------
    void InsertGraphDlg::ApplyGraphOptions(Graphs::Graph2D& graph) const
        {
        const auto applyLabel = [this](const GraphItems::Label& src, GraphItems::Label& dest)
        {
            if (src.GetText().empty())
                {
                return;
                }
            // apply all styling from the dialog's label to the graph's label
            dest.GetFont() = src.GetFont();
            dest.SetFontColor(src.GetFontColor());
            dest.SetFontBackgroundColor(src.GetFontBackgroundColor());
            dest.SetTextAlignment(src.GetTextAlignment());
            dest.SetLineSpacing(src.GetLineSpacing());
            dest.SetTextOrientation(src.GetTextOrientation());
            dest.SetPadding(src.GetTopPadding(), src.GetRightPadding(), src.GetBottomPadding(),
                            src.GetLeftPadding());
            dest.GetPen() = src.GetPen();
            dest.GetHeaderInfo() = src.GetHeaderInfo();
            // left image
            dest.SetLeftImage(src.GetLeftImage());
            const auto leftImgPath = src.GetPropertyTemplate(L"left-image.path");
            if (!leftImgPath.empty())
                {
                dest.SetPropertyTemplate(L"left-image.path", leftImgPath);
                }
            // top shapes
            const auto& topShapes = src.GetTopShape();
            if (topShapes.has_value())
                {
                dest.SetTopShape(topShapes.value(), src.GetTopImageOffset());
                }
            // expand constants if a report builder is available
            const auto rawText = src.GetText();
            const auto* builder = GetReportBuilder();
            if (builder != nullptr)
                {
                const auto expanded = builder->ExpandConstants(rawText);
                dest.SetText(expanded);
                if (expanded != rawText)
                    {
                    dest.SetPropertyTemplate(L"text", rawText);
                    }
                }
            else
                {
                dest.SetText(rawText);
                }
            // preserve any existing text template from the source
            const auto srcTemplate = src.GetPropertyTemplate(L"text");
            if (!srcTemplate.empty())
                {
                dest.SetPropertyTemplate(L"text", srcTemplate);
                }
        };

        applyLabel(m_titleLabel, graph.GetTitle());
        applyLabel(m_subtitleLabel, graph.GetSubtitle());
        applyLabel(m_captionLabel, graph.GetCaption());

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
            graph.SetPlotBackgroundImageFit(GetPlotBackgroundImageFit());

            // copy property templates to the graph for round-tripping
            for (const auto& [key, val] : bgImage.GetPropertyTemplates())
                {
                if (key.StartsWith(L"image-import.") || key.StartsWith(L"size."))
                    {
                    graph.SetPropertyTemplate(key, val);
                    }
                }
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

        // title, subtitle, caption — each opens a full Label editor
        auto* textSizer = new wxGridBagSizer(FromDIP(4), FromDIP(8));

        const auto previewColor = GetVariableLabelColor();

        textSizer->Add(new wxStaticText(graphPage, wxID_ANY, _(L"Title:")), wxGBPosition(0, 0),
                       wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
            {
            auto* btn = new wxButton(graphPage, wxID_ANY, _(L"Edit..."));
            btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnEditTitle(); });
            textSizer->Add(btn, wxGBPosition(0, 1));
            }
        m_titlePreview = new wxStaticText(graphPage, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                          wxDefaultSize, wxST_ELLIPSIZE_END);
        m_titlePreview->SetForegroundColour(previewColor);
        textSizer->Add(m_titlePreview, wxGBPosition(0, 2), wxDefaultSpan,
                       wxALIGN_CENTER_VERTICAL | wxEXPAND);

        textSizer->Add(new wxStaticText(graphPage, wxID_ANY, _(L"Subtitle:")), wxGBPosition(1, 0),
                       wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
            {
            auto* btn = new wxButton(graphPage, wxID_ANY, _(L"Edit..."));
            btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnEditSubtitle(); });
            textSizer->Add(btn, wxGBPosition(1, 1));
            }
        m_subtitlePreview = new wxStaticText(graphPage, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                             wxDefaultSize, wxST_ELLIPSIZE_END);
        m_subtitlePreview->SetForegroundColour(previewColor);
        textSizer->Add(m_subtitlePreview, wxGBPosition(1, 2), wxDefaultSpan,
                       wxALIGN_CENTER_VERTICAL | wxEXPAND);

        textSizer->Add(new wxStaticText(graphPage, wxID_ANY, _(L"Caption:")), wxGBPosition(2, 0),
                       wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
            {
            auto* btn = new wxButton(graphPage, wxID_ANY, _(L"Edit..."));
            btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnEditCaption(); });
            textSizer->Add(btn, wxGBPosition(2, 1));
            }
        m_captionPreview = new wxStaticText(graphPage, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                            wxDefaultSize, wxST_ELLIPSIZE_END);
        m_captionPreview->SetForegroundColour(previewColor);
        textSizer->Add(m_captionPreview, wxGBPosition(2, 2), wxDefaultSpan,
                       wxALIGN_CENTER_VERTICAL | wxEXPAND);
        textSizer->AddGrowableCol(2, 1);

        graphSizer->Add(textSizer, wxSizerFlags{}.Expand().Border());

        // background color
        auto* bgSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        bgSizer->Add(new wxStaticText(graphPage, wxID_ANY, _(L"Background color:")),
                     wxSizerFlags{}.CenterVertical());
        m_plotBgColorPicker = new wxColourPickerCtrl(graphPage, wxID_ANY, *wxWHITE);
        bgSizer->Add(m_plotBgColorPicker);

        graphSizer->Add(bgSizer, wxSizerFlags{}.Expand().Border());

        // background image
        auto* bgBox = new wxStaticBoxSizer(wxVERTICAL, graphPage, _(L"Background Image"));

        auto* bgImgSizer = new wxFlexGridSizer(3, wxSize{ FromDIP(8), FromDIP(4) });
        bgImgSizer->Add(new wxStaticText(bgBox->GetStaticBox(), wxID_ANY, _(L"Image:")),
                        wxSizerFlags{}.CenterVertical());
            {
            auto* btn = new wxButton(bgBox->GetStaticBox(), wxID_ANY, _(L"Edit..."));
            btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { OnEditBackgroundImage(); });
            bgImgSizer->Add(btn);
            }
        m_bgImagePreview = new wxStaticText(bgBox->GetStaticBox(), wxID_ANY, wxEmptyString,
                                            wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
        m_bgImagePreview->SetForegroundColour(previewColor);
        bgImgSizer->Add(m_bgImagePreview, wxSizerFlags{}.CenterVertical());
        bgBox->Add(bgImgSizer, wxSizerFlags{}.Expand().Border());

        auto* imgOptionsSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        imgOptionsSizer->Add(new wxStaticText(bgBox->GetStaticBox(), wxID_ANY, _(L"Opacity:")),
                             wxSizerFlags{}.CenterVertical());
            {
            auto* opacitySpin = new wxSpinCtrl(bgBox->GetStaticBox(), wxID_ANY);
            opacitySpin->SetRange(0, 255);
            opacitySpin->SetValue(255);
            opacitySpin->SetValidator(wxGenericValidator(&m_plotBgImageOpacity));
            imgOptionsSizer->Add(opacitySpin);
            }
        imgOptionsSizer->Add(new wxStaticText(bgBox->GetStaticBox(), wxID_ANY, _(L"Fit:")),
                             wxSizerFlags{}.CenterVertical());
            {
            auto* fitChoice =
                new wxChoice(bgBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                             nullptr, 0, wxGenericValidator(&m_plotBgImageFit));
            fitChoice->Append(_(L"Crop and center"));
            fitChoice->Append(_(L"Shrink"));
            imgOptionsSizer->Add(fitChoice);
            }
        bgBox->Add(imgOptionsSizer, wxSizerFlags{}.Border());

        graphSizer->Add(bgBox, wxSizerFlags{}.Expand().Border());

        // axis mirroring
        graphSizer->Add(new wxCheckBox(graphPage, wxID_ANY, _(L"Mirror X axis"), wxDefaultPosition,
                                       wxDefaultSize, 0, wxGenericValidator(&m_mirrorXAxis)),
                        wxSizerFlags{}.Border());

        graphSizer->Add(new wxCheckBox(graphPage, wxID_ANY, _(L"Mirror Y axis"), wxDefaultPosition,
                                       wxDefaultSize, 0, wxGenericValidator(&m_mirrorYAxis)),
                        wxSizerFlags{}.Border());
        }

    //-------------------------------------------
    void InsertGraphDlg::EditLabelHelper(GraphItems::Label& label, wxStaticText* preview,
                                         const wxString& dlgCaption)
        {
        InsertLabelDlg dlg(GetCanvas(), GetReportBuilder(), this, dlgCaption, wxID_ANY,
                           wxDefaultPosition, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                           label.GetText().empty() ? EditMode::Insert : EditMode::Edit,
                           false /*includePageOptions*/);
        if (!label.GetText().empty())
            {
            dlg.LoadFromLabel(label, GetCanvas());
            }
        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }
        dlg.ApplyToLabel(label);
        if (preview != nullptr)
            {
            preview->SetLabel(label.GetText());
            }
        }

    //-------------------------------------------
    void InsertGraphDlg::OnEditTitle()
        {
        EditLabelHelper(m_titleLabel, m_titlePreview, _(L"Edit Title"));
        }

    //-------------------------------------------
    void InsertGraphDlg::OnEditSubtitle()
        {
        EditLabelHelper(m_subtitleLabel, m_subtitlePreview, _(L"Edit Subtitle"));
        }

    //-------------------------------------------
    void InsertGraphDlg::OnEditCaption()
        {
        EditLabelHelper(m_captionLabel, m_captionPreview, _(L"Edit Caption"));
        }

    //-------------------------------------------
    void InsertGraphDlg::OnEditBackgroundImage()
        {
        InsertImageDlg dlg(GetCanvas(), GetReportBuilder(), this, _(L"Edit Background Image"),
                           wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                           m_plotBgImage.IsOk() ? EditMode::Edit : EditMode::Insert,
                           false /*includePageOptions*/);
        if (m_plotBgImage.IsOk())
            {
            dlg.LoadFromImage(m_plotBgImage, GetCanvas());
            }
        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        // rebuild the image from the dialog's settings
        const auto paths = dlg.GetImagePaths();
        if (paths.empty())
            {
            m_plotBgImage = GraphItems::Image{};
            if (m_bgImagePreview != nullptr)
                {
                m_bgImagePreview->SetLabel(wxEmptyString);
                }
            return;
            }

        // load and optionally stitch the image(s)
        std::vector<wxBitmap> bmps;
        for (const auto& path : paths)
            {
            auto loadedBmp = GraphItems::Image::LoadFile(path);
            if (loadedBmp.IsOk())
                {
                bmps.push_back(loadedBmp);
                }
            }
        if (bmps.empty())
            {
            return;
            }

        wxImage resultImg;
        if (bmps.size() == 1)
            {
            resultImg = bmps[0].ConvertToImage();
            }
        else if (dlg.GetStitchDirection() == L"vertical")
            {
            resultImg = GraphItems::Image::StitchVertically(bmps);
            }
        else
            {
            resultImg = GraphItems::Image::StitchHorizontally(bmps);
            }

        // apply effect
        const auto effect = dlg.GetImageEffect();
        if (effect != ImageEffect::NoEffect)
            {
            resultImg = GraphItems::Image::ApplyEffect(effect, resultImg);
            }

        m_plotBgImage = GraphItems::Image(resultImg);
        dlg.ApplyToImage(m_plotBgImage);

        // apply custom size
        if (dlg.IsCustomSizeEnabled())
            {
            const auto reqWidth = dlg.GetImageWidth();
            const auto reqHeight = dlg.GetImageHeight();
            const auto bestSz =
                GraphItems::Image::ToBestSize(resultImg.GetSize(), wxSize{ reqWidth, reqHeight });
            m_plotBgImage.SetSize(bestSz);
            }

        // cache property templates for round-tripping
        if (paths.GetCount() == 1)
            {
            m_plotBgImage.SetPropertyTemplate(L"image-import.path", paths[0]);
            }
        else
            {
            wxString joined;
            for (size_t i = 0; i < paths.GetCount(); ++i)
                {
                if (i > 0)
                    {
                    joined += L"\t";
                    }
                joined += paths[i];
                }
            m_plotBgImage.SetPropertyTemplate(L"image-import.paths", joined);
            m_plotBgImage.SetPropertyTemplate(L"image-import.stitch", dlg.GetStitchDirection());
            }

        // cache effect
        if (effect != ImageEffect::NoEffect)
            {
            const auto effectStr = ReportEnumConvert::ConvertImageEffectToString(effect);
            if (effectStr.has_value())
                {
                m_plotBgImage.SetPropertyTemplate(L"image-import.effect", effectStr.value());
                }
            }

        // cache custom size
        if (dlg.IsCustomSizeEnabled())
            {
            m_plotBgImage.SetPropertyTemplate(L"size.width", std::to_wstring(dlg.GetImageWidth()));
            m_plotBgImage.SetPropertyTemplate(L"size.height",
                                              std::to_wstring(dlg.GetImageHeight()));
            }

        // update preview
        if (m_bgImagePreview != nullptr)
            {
            const auto pathStr = m_plotBgImage.GetPropertyTemplate(L"image-import.path");
            if (!pathStr.empty())
                {
                m_bgImagePreview->SetLabel(wxFileName{ pathStr }.GetFullName());
                }
            else
                {
                m_bgImagePreview->SetLabel(
                    wxString::Format(wxPLURAL(L"%zu image", L"%zu images", paths.GetCount()),
                                     static_cast<size_t>(paths.GetCount())));
                }
            }
        }

    //-------------------------------------------
    wxColour InsertGraphDlg::GetPlotBackgroundColor() const
        {
        return (m_plotBgColorPicker != nullptr) ? m_plotBgColorPicker->GetColour() : wxColour{};
        }

    //-------------------------------------------
    bool InsertGraphDlg::GetMirrorXAxis() const { return m_mirrorXAxis; }

    //-------------------------------------------
    bool InsertGraphDlg::GetMirrorYAxis() const { return m_mirrorYAxis; }

    //-------------------------------------------
    void InsertGraphDlg::LoadGraphOptions(const Graphs::Graph2D& graph, const Canvas* canvas)
        {
        // copy the full Labels so we can round-trip all styling
        const auto loadLabel =
            [](const GraphItems::Label& src, GraphItems::Label& dest, wxStaticText* preview)
        {
            dest.GetFont() = src.GetFont();
            dest.SetFontColor(src.GetFontColor());
            dest.SetFontBackgroundColor(src.GetFontBackgroundColor());
            dest.SetTextAlignment(src.GetTextAlignment());
            dest.SetLineSpacing(src.GetLineSpacing());
            dest.SetTextOrientation(src.GetTextOrientation());
            dest.SetPadding(src.GetTopPadding(), src.GetRightPadding(), src.GetBottomPadding(),
                            src.GetLeftPadding());
            dest.GetPen() = src.GetPen();
            dest.GetHeaderInfo() = src.GetHeaderInfo();
            // left image
            dest.SetLeftImage(src.GetLeftImage());
            const auto leftImgPath = src.GetPropertyTemplate(L"left-image.path");
            if (!leftImgPath.empty())
                {
                dest.SetPropertyTemplate(L"left-image.path", leftImgPath);
                }
            // top shapes
            const auto& topShapes = src.GetTopShape();
            if (topShapes.has_value())
                {
                dest.SetTopShape(topShapes.value(), src.GetTopImageOffset());
                }
            // use the raw template text if available so constants are shown unexpanded
            const auto tmpl = src.GetPropertyTemplate(L"text");
            dest.SetText(tmpl.empty() ? src.GetText() : tmpl);
            if (!tmpl.empty())
                {
                dest.SetPropertyTemplate(L"text", tmpl);
                }
            if (preview != nullptr)
                {
                preview->SetLabel(src.GetText());
                }
        };
        loadLabel(graph.GetTitle(), m_titleLabel, m_titlePreview);
        loadLabel(graph.GetSubtitle(), m_subtitleLabel, m_subtitlePreview);
        loadLabel(graph.GetCaption(), m_captionLabel, m_captionPreview);

        const auto bgColor = graph.GetPlotBackgroundColor();
        if (bgColor.IsOk() && !bgColor.IsTransparent() && m_plotBgColorPicker != nullptr)
            {
            m_plotBgColorPicker->SetColour(bgColor);
            }

        // background image — rebuild from the graph's bitmap and property templates
        const auto& bgBundle = graph.GetPlotBackgroundImage();
        if (bgBundle.IsOk())
            {
            const auto bmp = bgBundle.GetBitmapFor(this);
            m_plotBgImage = GraphItems::Image(bmp.ConvertToImage());

            // copy property templates from the graph for round-tripping
            for (const auto& [key, val] : graph.GetPropertyTemplates())
                {
                if (key.StartsWith(L"image-import.") || key.StartsWith(L"size."))
                    {
                    m_plotBgImage.SetPropertyTemplate(key, val);
                    }
                }

            m_plotBgImageOpacity = graph.GetPlotBackgroundImageOpacity();
            m_plotBgImageFit = static_cast<int>(graph.GetPlotBackgroundImageFit());

            // update preview
            if (m_bgImagePreview != nullptr)
                {
                const auto pathStr = m_plotBgImage.GetPropertyTemplate(L"image-import.path");
                if (!pathStr.empty())
                    {
                    m_bgImagePreview->SetLabel(wxFileName{ pathStr }.GetFullName());
                    }
                else
                    {
                    m_bgImagePreview->SetLabel(_(L"(image)"));
                    }
                }
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
