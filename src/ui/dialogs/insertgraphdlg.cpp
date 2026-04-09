///////////////////////////////////////////////////////////////////////////////
// Name:        insertgraphdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertgraphdlg.h"
#include "../../app/wisteriaapp.h"
#include "../../base/reportenumconvert.h"
#include "../../graphs/graph2d.h"
#include "insertimgdlg.h"
#include "insertlabeldlg.h"
#include <wx/colordlg.h>
#include <wx/filename.h>
#include <wx/gbsizer.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertGraphDlg::InsertGraphDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                   wxWindow* parent, const wxString& caption, const wxWindowID id,
                                   const wxPoint& pos, const wxSize& size, const long style,
                                   EditMode editMode, GraphDlgOptions options)
        : InsertItemDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode),
          m_options(options)
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
                if (key.StartsWith(_DT(L"image-import.")) || key.StartsWith(_DT(L"size.")))
                    {
                    graph.SetPropertyTemplate(key, val);
                    }
                }
            }

        graph.MirrorXAxis(GetMirrorXAxis());
        graph.MirrorYAxis(GetMirrorYAxis());

        // apply color/brush scheme
        if (m_options & GraphDlgIncludeColorScheme)
            {
            if (IsUsingCustomColors())
                {
                const auto& colors = GetCustomColors();
                auto colorScheme = std::make_shared<Colors::Schemes::ColorScheme>(colors);
                graph.SetColorScheme(colorScheme);
                graph.SetBrushScheme(std::make_shared<Brushes::Schemes::BrushScheme>(*colorScheme));
                }
            else if (GetColorScheme() != nullptr)
                {
                graph.SetColorScheme(GetColorScheme());
                graph.SetBrushScheme(
                    std::make_shared<Brushes::Schemes::BrushScheme>(*GetColorScheme()));
                }
            }
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
    bool InsertGraphDlg::ConfirmOverwrite()
        {
        if (!InsertItemDlg::ConfirmOverwrite())
            {
            return false;
            }

        const auto legendPlace = GetLegendPlacement();
        if (GetEditMode() != EditMode::Insert || legendPlace == LegendPlacement::None ||
            GetCanvas() == nullptr)
            {
            return true;
            }

        const auto [gridRows, gridCols] = GetCanvas()->GetFixedObjectsGridSize();
        const auto graphRow = GetSelectedRow();
        const auto graphCol = GetSelectedColumn();

        // determine the cell where the legend would be placed
        // (mirrors the logic in WisteriaView::PlaceGraphWithLegend)
        size_t legendRow = graphRow;
        size_t legendCol = graphCol;
        bool legendCellExists = false;

        if (legendPlace == LegendPlacement::Right)
            {
            legendCol = graphCol + 1;
            legendCellExists = (legendCol < gridCols);
            }
        else if (legendPlace == LegendPlacement::Left)
            {
            if (graphCol > 0)
                {
                legendCol = graphCol - 1;
                legendCellExists = true;
                }
            // graphCol == 0: grid will be expanded, no existing cell to overwrite
            }
        else if (legendPlace == LegendPlacement::Bottom)
            {
            legendRow = graphRow + 1;
            legendCellExists = (legendRow < gridRows);
            }
        else if (legendPlace == LegendPlacement::Top)
            {
            if (graphRow > 0)
                {
                legendRow = graphRow - 1;
                legendCellExists = true;
                }
            // graphRow == 0: grid will be expanded, no existing cell to overwrite
            }

        if (legendCellExists && GetCanvas()->GetFixedObject(legendRow, legendCol) != nullptr)
            {
            if (wxMessageBox(_(L"The legend cell already contains an "
                               "item. Do you want to replace it?"),
                             _(L"Replace Item"), wxYES_NO | wxICON_QUESTION, this) != wxYES)
                {
                return false;
                }
            }

        return true;
        }

    //-------------------------------------------
    void InsertGraphDlg::CreateGraphOptionsPage()
        {
        auto* graphPage = new wxPanel(GetSideBarBook());
        auto* graphSizer = new wxBoxSizer(wxVERTICAL);
        graphPage->SetSizer(graphSizer);
        GetSideBarBook()->AddPage(graphPage, _(L"General Graph Options"), ID_GRAPH_OPTIONS_SECTION,
                                  false);

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
        m_titlePreview = new wxStaticText(graphPage, wxID_ANY, wxString{}, wxDefaultPosition,
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
        m_subtitlePreview = new wxStaticText(graphPage, wxID_ANY, wxString{}, wxDefaultPosition,
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
        m_captionPreview = new wxStaticText(graphPage, wxID_ANY, wxString{}, wxDefaultPosition,
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
        m_bgImagePreview = new wxStaticText(bgBox->GetStaticBox(), wxID_ANY, wxString{},
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

        graphSizer->Add(bgBox, wxSizerFlags{}.Border());

        // color scheme / custom colors
        if (m_options & GraphDlgIncludeColorScheme)
            {
            auto* colorBox = new wxStaticBoxSizer(wxVERTICAL, graphPage, _(L"Colors"));

            // radio: named scheme + choice on same row
            auto* namedSchemeSizer = new wxBoxSizer(wxHORIZONTAL);
            m_namedSchemeRadio =
                new wxRadioButton(colorBox->GetStaticBox(), wxID_ANY, _(L"Color scheme:"),
                                  wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
            namedSchemeSizer->Add(m_namedSchemeRadio, wxSizerFlags{}.CenterVertical());
            m_colorSchemeChoice =
                new wxChoice(colorBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             GetColorSchemeNames(), 0, wxGenericValidator(&m_colorSchemeIndex));
            namedSchemeSizer->Add(m_colorSchemeChoice,
                                  wxSizerFlags{}.CenterVertical().Border(wxLEFT));
            colorBox->Add(namedSchemeSizer, wxSizerFlags{}.Border());

            // radio: custom colors
            m_customColorsRadio =
                new wxRadioButton(colorBox->GetStaticBox(), wxID_ANY, _(L"Custom color list:"));
            colorBox->Add(m_customColorsRadio, wxSizerFlags{}.Border());

            m_customColorListBox = new wxEditableListBox(
                colorBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                wxSize{ FromDIP(300), FromDIP(120) },
                wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
            RefreshCustomColorList();
            colorBox->Add(m_customColorListBox,
                          wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

            // override New to open a color picker
            m_customColorListBox->GetNewButton()->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
                                                       { OnAddCustomColor(); });
            m_customColorListBox->GetNewButton()->SetBitmapLabel(
                wxGetApp().ReadSvgIcon(L"color-wheel.svg", wxSize{ 16, 16 }));

            // override Edit to open a color picker for the selected item
            m_customColorListBox->GetEditButton()->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
                                                        { OnEditCustomColor(); });

            // override Delete to remove the selected color from our vector
            m_customColorListBox->GetDelButton()->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
                                                       { OnRemoveCustomColor(); });

            graphSizer->Add(colorBox, wxSizerFlags{}.Border());

            // initial enable state — named scheme selected by default
            m_namedSchemeRadio->SetValue(true);
            m_colorSchemeChoice->Enable(true);
            m_customColorListBox->Enable(false);

            m_namedSchemeRadio->Bind(wxEVT_RADIOBUTTON, [this]([[maybe_unused]] wxCommandEvent&)
                                     { OnColorModeChanged(); });
            m_customColorsRadio->Bind(wxEVT_RADIOBUTTON, [this]([[maybe_unused]] wxCommandEvent&)
                                      { OnColorModeChanged(); });
            }
        }

    //-------------------------------------------
    void InsertGraphDlg::CreateAxisOptionsPage()
        {
        auto* axisPage = new wxPanel(GetSideBarBook());
        auto* axisSizer = new wxBoxSizer(wxVERTICAL);
        axisPage->SetSizer(axisSizer);
        GetSideBarBook()->AddPage(axisPage, _(L"Axis Options"), ID_AXIS_OPTIONS_SECTION, true);

        axisSizer->Add(new wxCheckBox(axisPage, wxID_ANY, _(L"Mirror X axis"), wxDefaultPosition,
                                      wxDefaultSize, 0, wxGenericValidator(&m_mirrorXAxis)),
                       wxSizerFlags{}.Border());

        axisSizer->Add(new wxCheckBox(axisPage, wxID_ANY, _(L"Mirror Y axis"), wxDefaultPosition,
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
            dlg.LoadFromLabel(label);
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
        InsertImageDlg dlg(
            GetCanvas(), GetReportBuilder(), this, _(L"Edit Background Image"), wxID_ANY,
            wxDefaultPosition, wxDefaultSize,
            wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
            m_plotBgImage.IsOk() ? EditMode::Edit : EditMode::Insert,
            // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
            static_cast<ImageDlgOptions>(ImageDlgIncludeAll & ~ImageDlgIncludePageOptions));
        if (m_plotBgImage.IsOk())
            {
            dlg.LoadFromImage(m_plotBgImage);
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
                m_bgImagePreview->SetLabel(wxString{});
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
        else if (dlg.GetStitchDirection() == Wisteria::Orientation::Vertical)
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
            m_plotBgImage.SetPropertyTemplate(
                L"image-import.stitch",
                (dlg.GetStitchDirection() == Wisteria::Orientation::Vertical) ? L"vertical" :
                                                                                L"horizontal");
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
    void InsertGraphDlg::OnAddCustomColor()
        {
        wxColourData colourData;
        wxColourDialog dlg(this, &colourData);
        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        m_customColors.push_back(dlg.GetColourData().GetColour());
        RefreshCustomColorList();
        }

    //-------------------------------------------
    void InsertGraphDlg::OnEditCustomColor()
        {
        const auto sel = m_customColorListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                          wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_customColors.size()))
            {
            return;
            }

        wxColourData colourData;
        colourData.SetColour(m_customColors[static_cast<size_t>(sel)]);

        wxColourDialog dlg(this, &colourData);
        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        m_customColors[static_cast<size_t>(sel)] = dlg.GetColourData().GetColour();
        RefreshCustomColorList();
        }

    //-------------------------------------------
    void InsertGraphDlg::OnRemoveCustomColor()
        {
        const auto sel = m_customColorListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                          wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_customColors.size()))
            {
            return;
            }

        m_customColors.erase(m_customColors.begin() + sel);
        RefreshCustomColorList();
        }

    //-------------------------------------------
    void InsertGraphDlg::RefreshCustomColorList()
        {
        wxArrayString strings;
        strings.reserve(m_customColors.size());
        for (const auto& clr : m_customColors)
            {
            strings.Add(clr.GetAsString(wxC2S_HTML_SYNTAX));
            }
        m_customColorListBox->SetStrings(strings);

        auto* listCtrl = m_customColorListBox->GetListCtrl();
        for (long i = 0; std::cmp_less(i, m_customColors.size()); ++i)
            {
            listCtrl->SetItemTextColour(i, m_customColors[static_cast<size_t>(i)]);
            }
        }

    //-------------------------------------------
    void InsertGraphDlg::OnColorModeChanged()
        {
        const bool useCustom = m_customColorsRadio->GetValue();
        m_useCustomColors = useCustom;
        m_colorSchemeChoice->Enable(!useCustom);
        m_customColorListBox->Enable(useCustom);
        }

    //-------------------------------------------
    bool InsertGraphDlg::ValidateColorScheme()
        {
        if (!(m_options & GraphDlgIncludeColorScheme))
            {
            return true;
            }
        if (m_useCustomColors && m_customColors.empty())
            {
            wxMessageBox(_(L"Please enter at least one color."), _(L"No Colors Specified"),
                         wxOK | wxICON_WARNING, this);
            return false;
            }
        return true;
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
    void InsertGraphDlg::LoadGraphOptions(const Graphs::Graph2D& graph)
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
                if (key.StartsWith(_DT(L"image-import.")) || key.StartsWith(_DT(L"size.")))
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

        // save all four axes for round-tripping (SetData() rebuilds them)
        m_savedAxes.clear();
        for (const auto axisType : { AxisType::BottomXAxis, AxisType::TopXAxis, AxisType::LeftYAxis,
                                     AxisType::RightYAxis })
            {
            m_savedAxes.emplace(axisType, graph.GetAxis(axisType));
            }

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

        // color scheme
        if (m_options & GraphDlgIncludeColorScheme)
            {
            const auto& brushScheme = graph.GetBrushScheme();
            const auto namedIndex = ColorSchemeToIndex(graph.GetColorScheme());
            if (namedIndex == 0 && brushScheme != nullptr && !brushScheme->GetBrushes().empty())
                {
                m_useCustomColors = true;
                m_customColors.clear();
                for (const auto& brush : brushScheme->GetBrushes())
                    {
                    m_customColors.push_back(brush.GetColour());
                    }
                m_customColorsRadio->SetValue(true);
                RefreshCustomColorList();
                m_colorSchemeIndex = 0;
                }
            else
                {
                m_useCustomColors = false;
                m_colorSchemeIndex = namedIndex;
                m_namedSchemeRadio->SetValue(true);
                }
            OnColorModeChanged();
            }

        LoadPageOptions(graph);
        }

    //-------------------------------------------
    wxArrayString InsertGraphDlg::GetColorSchemeNames()
        {
        wxArrayString names;
        names.Add(_(L"Default"));
        names.Add(_(L"Arctic Chill"));
        names.Add(_(L"Back to School"));
        names.Add(_(L"Box of Chocolates"));
        names.Add(_(L"Campfire"));
        names.Add(_(L"Coffee Shop"));
        names.Add(_(L"Cosmopolitan"));
        names.Add(_(L"Day and Night"));
        names.Add(_(L"Decade 1920s"));
        names.Add(_(L"Decade 1940s"));
        names.Add(_(L"Decade 1950s"));
        names.Add(_(L"Decade 1960s"));
        names.Add(_(L"Decade 1970s"));
        names.Add(_(L"Decade 1980s"));
        names.Add(_(L"Decade 1990s"));
        names.Add(_(L"Decade 2000s"));
        names.Add(_(L"Dusk"));
        names.Add(_(L"Earth Tones"));
        names.Add(_(L"Fresh Flowers"));
        names.Add(_(L"Ice Cream"));
        names.Add(_(L"Meadow Sunset"));
        names.Add(_(L"Nautical"));
        names.Add(_(L"October"));
        names.Add(_(L"Produce Section"));
        names.Add(_(L"Rolling Thunder"));
        names.Add(_(L"Seasons"));
        names.Add(_(L"Semesters"));
        names.Add(_(L"Shabby Chic"));
        names.Add(_(L"Slytherin"));
        names.Add(_(L"Spring"));
        names.Add(_(L"Tasty Waves"));
        names.Add(_(L"Typewriter"));
        names.Add(_(L"Urban Oasis"));
        return names;
        }

    //-------------------------------------------
    // clang-format off
    std::shared_ptr<Colors::Schemes::ColorScheme>
    InsertGraphDlg::ColorSchemeFromIndex(const int index)
        {
        switch (index)
            {
        case 1:  return std::make_shared<Colors::Schemes::ArcticChill>();
        case 2:  return std::make_shared<Colors::Schemes::BackToSchool>();
        case 3:  return std::make_shared<Colors::Schemes::BoxOfChocolates>();
        case 4:  return std::make_shared<Colors::Schemes::Campfire>();
        case 5:  return std::make_shared<Colors::Schemes::CoffeeShop>();
        case 6:  return std::make_shared<Colors::Schemes::Cosmopolitan>();
        case 7:  return std::make_shared<Colors::Schemes::DayAndNight>();
        case 8:  return std::make_shared<Colors::Schemes::Decade1920s>();
        case 9:  return std::make_shared<Colors::Schemes::Decade1940s>();
        case 10: return std::make_shared<Colors::Schemes::Decade1950s>();
        case 11: return std::make_shared<Colors::Schemes::Decade1960s>();
        case 12: return std::make_shared<Colors::Schemes::Decade1970s>();
        case 13: return std::make_shared<Colors::Schemes::Decade1980s>();
        case 14: return std::make_shared<Colors::Schemes::Decade1990s>();
        case 15: return std::make_shared<Colors::Schemes::Decade2000s>();
        case 16: return std::make_shared<Colors::Schemes::Dusk>();
        case 17: return std::make_shared<Colors::Schemes::EarthTones>();
        case 18: return std::make_shared<Colors::Schemes::FreshFlowers>();
        case 19: return std::make_shared<Colors::Schemes::IceCream>();
        case 20: return std::make_shared<Colors::Schemes::MeadowSunset>();
        case 21: return std::make_shared<Colors::Schemes::Nautical>();
        case 22: return std::make_shared<Colors::Schemes::October>();
        case 23: return std::make_shared<Colors::Schemes::ProduceSection>();
        case 24: return std::make_shared<Colors::Schemes::RollingThunder>();
        case 25: return std::make_shared<Colors::Schemes::Seasons>();
        case 26: return std::make_shared<Colors::Schemes::Semesters>();
        case 27: return std::make_shared<Colors::Schemes::ShabbyChic>();
        case 28: return std::make_shared<Colors::Schemes::Slytherin>();
        case 29: return std::make_shared<Colors::Schemes::Spring>();
        case 30: return std::make_shared<Colors::Schemes::TastyWaves>();
        case 31: return std::make_shared<Colors::Schemes::Typewriter>();
        case 32: return std::make_shared<Colors::Schemes::UrbanOasis>();
        case 0:  [[fallthrough]];
        default: return nullptr;
            }
        }

    //-------------------------------------------
    int InsertGraphDlg::ColorSchemeToIndex(
        const std::shared_ptr<Colors::Schemes::ColorScheme>& scheme)
        {
        if (scheme == nullptr)
            { return 0; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::ArcticChill)))
            { return 1; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::BackToSchool)))
            { return 2; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::BoxOfChocolates)))
            { return 3; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Campfire)))
            { return 4; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::CoffeeShop)))
            { return 5; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Cosmopolitan)))
            { return 6; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::DayAndNight)))
            { return 7; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Decade1920s)))
            { return 8; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Decade1940s)))
            { return 9; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Decade1950s)))
            { return 10; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Decade1960s)))
            { return 11; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Decade1970s)))
            { return 12; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Decade1980s)))
            { return 13; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Decade1990s)))
            { return 14; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Decade2000s)))
            { return 15; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Dusk)))
            { return 16; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::EarthTones)))
            { return 17; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::FreshFlowers)))
            { return 18; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::IceCream)))
            { return 19; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::MeadowSunset)))
            { return 20; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Nautical)))
            { return 21; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::October)))
            { return 22; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::ProduceSection)))
            { return 23; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::RollingThunder)))
            { return 24; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Seasons)))
            { return 25; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Semesters)))
            { return 26; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::ShabbyChic)))
            { return 27; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Slytherin)))
            { return 28; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Spring)))
            { return 29; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::TastyWaves)))
            { return 30; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::Typewriter)))
            { return 31; }
        if (scheme->IsKindOf(wxCLASSINFO(Colors::Schemes::UrbanOasis)))
            { return 32; }
        return 0;
        }

    //-------------------------------------------
    wxString InsertGraphDlg::ColorSchemeToName(const int index)
        {
        switch (index)
            {
        case 1:  return wxString{ L"arcticchill" };
        case 2:  return wxString{ L"backtoschool" };
        case 3:  return wxString{ L"boxofchocolates" };
        case 4:  return wxString{ L"campfire" };
        case 5:  return wxString{ L"coffeeshop" };
        case 6:  return wxString{ L"cosmopolitan" };
        case 7:  return wxString{ L"dayandnight" };
        case 8:  return wxString{ L"decade1920s" };
        case 9:  return wxString{ L"decade1940s" };
        case 10: return wxString{ L"decade1950s" };
        case 11: return wxString{ L"decade1960s" };
        case 12: return wxString{ L"decade1970s" };
        case 13: return wxString{ L"decade1980s" };
        case 14: return wxString{ L"decade1990s" };
        case 15: return wxString{ L"decade2000s" };
        case 16: return wxString{ L"dusk" };
        case 17: return wxString{ L"earthtones" };
        case 18: return wxString{ L"freshflowers" };
        case 19: return wxString{ L"icecream" };
        case 20: return wxString{ L"meadowsunset" };
        case 21: return wxString{ L"nautical" };
        case 22: return wxString{ L"october" };
        case 23: return wxString{ L"producesection" };
        case 24: return wxString{ L"rollingthunder" };
        case 25: return wxString{ L"seasons" };
        case 26: return wxString{ L"semesters" };
        case 27: return wxString{ L"shabbychic" };
        case 28: return wxString{ L"slytherin" };
        case 29: return wxString{ L"spring" };
        case 30: return wxString{ L"tastywaves" };
        case 31: return wxString{ L"typewriter" };
        case 32: return wxString{ L"urbanoasis" };
        case 0:  [[fallthrough]];
        default: return wxString{};
            }
        }

    //-------------------------------------------
    void InsertGraphDlg::ApplyAxisOverrides(Graphs::Graph2D& graph) const
        {
        for (const auto& [axisType, saved] : m_savedAxes)
            {
            graph.GetAxis(axisType) = saved;
            }
        }

    // clang-format on
    } // namespace Wisteria::UI
