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
#include <wx/choicdlg.h>
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

        // replace annotations (clear first to avoid
        // duplicating existing entries when editing a graph)
        graph.ClearAnnotations();
        for (const auto& ann : m_annotations)
            {
            auto label = std::make_shared<GraphItems::Label>(ann.label);
            // expand constants and set template for round-tripping,
            // mirroring how titles/subtitles/captions are handled
            const auto rawText = label->GetText();
            const auto* builder = GetReportBuilder();
            if (builder != nullptr)
                {
                const auto expanded = builder->ExpandConstants(rawText);
                label->SetText(expanded);
                if (expanded != rawText)
                    {
                    label->SetPropertyTemplate(L"text", rawText);
                    }
                else
                    {
                    // user changed the text to a literal value;
                    // clear any stale template so serialization
                    // picks up the actual text
                    label->SetPropertyTemplate(L"text", wxString{});
                    }
                }
            graph.AddAnnotation(label, ann.anchor, ann.interestPts);
            }

        // replace reference lines and areas
        graph.GetReferenceLines().clear();
        for (const auto& rl : m_referenceLines)
            {
            graph.AddReferenceLine(rl);
            }
        graph.GetReferenceAreas().clear();
        for (const auto& ra : m_referenceAreas)
            {
            graph.AddReferenceArea(ra);
            }

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

        // apply shape scheme
        if (m_options & GraphDlgIncludeShapeScheme)
            {
            graph.SetShapeScheme(GetShapeScheme());
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
        GetSideBarBook()->AddPage(graphPage, _(L"General"), ID_GRAPH_OPTIONS_SECTION, false);

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

        // shape scheme
        if (m_options & GraphDlgIncludeShapeScheme)
            {
            auto* shapeBox = new wxStaticBoxSizer(wxVERTICAL, graphPage, _(L"Point Shapes"));

            // radio: named scheme + choice on same row
            auto* namedShapeSizer = new wxBoxSizer(wxHORIZONTAL);
            m_namedShapeRadio =
                new wxRadioButton(shapeBox->GetStaticBox(), wxID_ANY, _(L"Shape scheme:"),
                                  wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
            namedShapeSizer->Add(m_namedShapeRadio, wxSizerFlags{}.CenterVertical());
            m_shapeSchemeChoice =
                new wxChoice(shapeBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             GetShapeSchemeNames(), 0, wxGenericValidator(&m_shapeSchemeIndex));
            namedShapeSizer->Add(m_shapeSchemeChoice,
                                 wxSizerFlags{}.CenterVertical().Border(wxLEFT));
            shapeBox->Add(namedShapeSizer, wxSizerFlags{}.Border());

            // radio: custom shape list
            m_customShapeRadio =
                new wxRadioButton(shapeBox->GetStaticBox(), wxID_ANY, _(L"Custom shape list:"));
            shapeBox->Add(m_customShapeRadio, wxSizerFlags{}.Border());

            m_customShapeListBox = new wxEditableListBox(
                shapeBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                wxSize{ FromDIP(300), FromDIP(120) },
                wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
            RefreshCustomShapeList();
            shapeBox->Add(m_customShapeListBox,
                          wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

            // override New to open a shape picker
            m_customShapeListBox->GetNewButton()->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
                                                       { OnAddCustomShape(); });
            m_customShapeListBox->GetNewButton()->SetBitmapLabel(
                wxGetApp().ReadSvgIcon(L"shape.svg", wxSize{ 16, 16 }));

            // override Edit to open a shape picker for the selected item
            m_customShapeListBox->GetEditButton()->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
                                                        { OnEditCustomShape(); });

            // override Delete to remove the selected shape
            m_customShapeListBox->GetDelButton()->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
                                                       { OnRemoveCustomShape(); });

            graphSizer->Add(shapeBox, wxSizerFlags{}.Border());

            // initial enable state — named scheme selected by default
            m_namedShapeRadio->SetValue(true);
            m_shapeSchemeChoice->Enable(true);
            m_customShapeListBox->Enable(false);

            m_namedShapeRadio->Bind(wxEVT_RADIOBUTTON, [this]([[maybe_unused]] wxCommandEvent&)
                                    { OnShapeModeChanged(); });
            m_customShapeRadio->Bind(wxEVT_RADIOBUTTON, [this]([[maybe_unused]] wxCommandEvent&)
                                     { OnShapeModeChanged(); });
            }
        }

    //-------------------------------------------
    void InsertGraphDlg::CreateAnnotationsPage()
        {
        auto* annotPage = new wxPanel(GetSideBarBook());
        auto* annotSizer = new wxBoxSizer(wxVERTICAL);
        annotPage->SetSizer(annotSizer);
        GetSideBarBook()->AddPage(annotPage, _(L"Annotations & References"), ID_ANNOTATIONS_SECTION,
                                  true);

        // Annotations
        //------------
        auto* annotBox = new wxStaticBoxSizer(wxVERTICAL, annotPage, _(L"Annotations"));
        m_annotationListBox = new wxEditableListBox(
            annotBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(100) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        RefreshAnnotationList();
        annotBox->Add(m_annotationListBox, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

        m_annotationListBox->GetNewButton()->Bind(wxEVT_BUTTON,
                                                  [this](wxCommandEvent&) { OnAddAnnotation(); });
        m_annotationListBox->GetNewButton()->SetBitmapLabel(
            wxGetApp().ReadSvgIcon(L"label.svg", wxSize{ 16, 16 }));
        m_annotationListBox->GetEditButton()->Bind(wxEVT_BUTTON,
                                                   [this](wxCommandEvent&) { OnEditAnnotation(); });
        m_annotationListBox->GetDelButton()->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
                                                  { OnRemoveAnnotation(); });
        m_annotationListBox->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                                  [this](wxListEvent&) { OnEditAnnotation(); });
        annotSizer->Add(annotBox, wxSizerFlags{ 1 }.Expand().Border());

        // Reference Lines
        //----------------
        auto* refLineBox = new wxStaticBoxSizer(wxVERTICAL, annotPage, _(L"Reference Lines"));
        m_refLineListBox = new wxEditableListBox(
            refLineBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(100) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        RefreshReferenceLineList();
        refLineBox->Add(m_refLineListBox, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

        m_refLineListBox->GetNewButton()->Bind(wxEVT_BUTTON,
                                               [this](wxCommandEvent&) { OnAddReferenceLine(); });
        m_refLineListBox->GetNewButton()->SetBitmapLabel(
            wxGetApp().ReadSvgIcon(L"label.svg", wxSize{ 16, 16 }));
        m_refLineListBox->GetEditButton()->Bind(wxEVT_BUTTON,
                                                [this](wxCommandEvent&) { OnEditReferenceLine(); });
        m_refLineListBox->GetDelButton()->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
                                               { OnRemoveReferenceLine(); });
        m_refLineListBox->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                               [this](wxListEvent&) { OnEditReferenceLine(); });
        annotSizer->Add(refLineBox, wxSizerFlags{ 1 }.Expand().Border());

        // Reference Areas
        //----------------
        auto* refAreaBox = new wxStaticBoxSizer(wxVERTICAL, annotPage, _(L"Reference Areas"));
        m_refAreaListBox = new wxEditableListBox(
            refAreaBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(100) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        RefreshReferenceAreaList();
        refAreaBox->Add(m_refAreaListBox, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

        m_refAreaListBox->GetNewButton()->Bind(wxEVT_BUTTON,
                                               [this](wxCommandEvent&) { OnAddReferenceArea(); });
        m_refAreaListBox->GetNewButton()->SetBitmapLabel(
            wxGetApp().ReadSvgIcon(L"label.svg", wxSize{ 16, 16 }));
        m_refAreaListBox->GetEditButton()->Bind(wxEVT_BUTTON,
                                                [this](wxCommandEvent&) { OnEditReferenceArea(); });
        m_refAreaListBox->GetDelButton()->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
                                               { OnRemoveReferenceArea(); });
        m_refAreaListBox->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                               [this](wxListEvent&) { OnEditReferenceArea(); });
        annotSizer->Add(refAreaBox, wxSizerFlags{ 1 }.Expand().Border());
        }

    //-------------------------------------------
    void InsertGraphDlg::CreateAxisOptionsPage()
        {
        auto* axisPage = new wxPanel(GetSideBarBook());
        auto* axisSizer = new wxBoxSizer(wxVERTICAL);
        axisPage->SetSizer(axisSizer);
        GetSideBarBook()->AddPage(axisPage, _(L"Axes"), ID_AXIS_OPTIONS_SECTION, true);

        // axis selector
        auto* selectorSizer = new wxBoxSizer(wxHORIZONTAL);
        selectorSizer->Add(new wxStaticText(axisPage, wxID_ANY, _(L"Edit axis:")),
                           wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_axisSelector = new wxChoice(axisPage, wxID_ANY);
        m_axisSelector->Append(_(L"Bottom X"));
        m_axisSelector->Append(_(L"Top X"));
        m_axisSelector->Append(_(L"Left Y"));
        m_axisSelector->Append(_(L"Right Y"));
        m_axisSelector->SetSelection(0);
        m_axisSelector->Bind(wxEVT_CHOICE, [this](wxCommandEvent&) { OnAxisSelectionChanged(); });
        selectorSizer->Add(m_axisSelector, wxSizerFlags{ 1 }.Expand());
        axisSizer->Add(selectorSizer, wxSizerFlags{}.Expand().Border());

        auto* gridSizer = new wxGridBagSizer(FromDIP(4), FromDIP(4));

        // Group 1: Title / Header / Footer
        //------------------
        auto* titleBox = new wxStaticBoxSizer(wxVERTICAL, axisPage, _(L"Title / Header / Footer"));
        auto* titleGrid = new wxFlexGridSizer(2, FromDIP(4), FromDIP(4));
        titleGrid->AddGrowableCol(0, 1);
        titleBox->Add(titleGrid, wxSizerFlags{}.Expand().Border());

        titleGrid->Add(new wxStaticText(titleBox->GetStaticBox(), wxID_ANY, _(L"Title:")),
                       wxSizerFlags{}.CenterVertical());
        auto* editTitleBtn = new wxButton(titleBox->GetStaticBox(), wxID_ANY, _(L"Edit..."));
        editTitleBtn->Bind(wxEVT_BUTTON,
                           [this](wxCommandEvent&)
                           {
                               auto& axis = m_savedAxes.at(m_currentAxisType);
                               EditLabelHelper(axis.GetTitle(), nullptr, _(L"Edit Axis Title"));
                           });
        titleGrid->Add(editTitleBtn);

        titleGrid->Add(new wxStaticText(titleBox->GetStaticBox(), wxID_ANY, _(L"Header:")),
                       wxSizerFlags{}.CenterVertical());
        auto* editHeaderBtn = new wxButton(titleBox->GetStaticBox(), wxID_ANY, _(L"Edit..."));
        editHeaderBtn->Bind(wxEVT_BUTTON,
                            [this](wxCommandEvent&)
                            {
                                auto& axis = m_savedAxes.at(m_currentAxisType);
                                EditLabelHelper(axis.GetHeader(), nullptr, _(L"Edit Axis Header"));
                            });
        titleGrid->Add(editHeaderBtn);

        titleGrid->Add(new wxStaticText(titleBox->GetStaticBox(), wxID_ANY, _(L"Footer:")),
                       wxSizerFlags{}.CenterVertical());
        auto* editFooterBtn = new wxButton(titleBox->GetStaticBox(), wxID_ANY, _(L"Edit..."));
        editFooterBtn->Bind(wxEVT_BUTTON,
                            [this](wxCommandEvent&)
                            {
                                auto& axis = m_savedAxes.at(m_currentAxisType);
                                EditLabelHelper(axis.GetFooter(), nullptr, _(L"Edit Axis Footer"));
                            });
        titleGrid->Add(editFooterBtn);

        gridSizer->Add(titleBox, wxGBPosition(0, 0), wxDefaultSpan, wxEXPAND | wxALL, FromDIP(4));

        // Group 2: Axis Line
        //------------------
        auto* lineBox = new wxStaticBoxSizer(wxVERTICAL, axisPage, _(L"Axis Line"));
        auto* lineGrid = new wxFlexGridSizer(2, FromDIP(4), FromDIP(4));
        lineGrid->AddGrowableCol(1, 1);
        lineBox->Add(lineGrid, wxSizerFlags{}.Expand().Border());

        lineGrid->Add(new wxStaticText(lineBox->GetStaticBox(), wxID_ANY, _(L"Color:")),
                      wxSizerFlags{}.CenterVertical());
        m_axisLineColorPicker = new wxColourPickerCtrl(lineBox->GetStaticBox(), wxID_ANY);
        lineGrid->Add(m_axisLineColorPicker, wxSizerFlags{}.Expand());

        lineGrid->Add(new wxStaticText(lineBox->GetStaticBox(), wxID_ANY, _(L"Width:")),
                      wxSizerFlags{}.CenterVertical());
        m_axisLineWidthSpin =
            new wxSpinCtrl(lineBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 1);
        lineGrid->Add(m_axisLineWidthSpin, wxSizerFlags{}.Expand());

        lineGrid->Add(new wxStaticText(lineBox->GetStaticBox(), wxID_ANY, _(L"Style:")),
                      wxSizerFlags{}.CenterVertical());
        m_axisLineStyleChoice = new wxChoice(lineBox->GetStaticBox(), wxID_ANY);
        m_axisLineStyleChoice->Append(_(L"None"));
        m_axisLineStyleChoice->Append(_(L"Solid"));
        m_axisLineStyleChoice->Append(_(L"Dot"));
        m_axisLineStyleChoice->Append(_(L"Long dash"));
        m_axisLineStyleChoice->Append(_(L"Short dash"));
        m_axisLineStyleChoice->Append(_(L"Dot dash"));
        lineGrid->Add(m_axisLineStyleChoice, wxSizerFlags{}.Expand());

        lineGrid->Add(new wxStaticText(lineBox->GetStaticBox(), wxID_ANY, _(L"Cap:")),
                      wxSizerFlags{}.CenterVertical());
        m_axisCapStyleChoice = new wxChoice(lineBox->GetStaticBox(), wxID_ANY);
        m_axisCapStyleChoice->Append(_(L"No cap"));
        m_axisCapStyleChoice->Append(_(L"Arrow"));
        lineGrid->Add(m_axisCapStyleChoice, wxSizerFlags{}.Expand());

        m_axisReverseCheck = new wxCheckBox(lineBox->GetStaticBox(), wxID_ANY, _(L"Reverse"));
        lineBox->Add(m_axisReverseCheck, wxSizerFlags{}.Border());

        gridSizer->Add(lineBox, wxGBPosition(1, 0), wxDefaultSpan, wxEXPAND | wxALL, FromDIP(4));

        // Group 3: Gridlines
        //------------------
        auto* gridlineBox = new wxStaticBoxSizer(wxVERTICAL, axisPage, _(L"Gridlines"));
        auto* gridlineGrid = new wxFlexGridSizer(2, FromDIP(4), FromDIP(4));
        gridlineGrid->AddGrowableCol(1, 1);
        gridlineBox->Add(gridlineGrid, wxSizerFlags{}.Expand().Border());

        gridlineGrid->Add(new wxStaticText(gridlineBox->GetStaticBox(), wxID_ANY, _(L"Color:")),
                          wxSizerFlags{}.CenterVertical());
        m_gridlineColorPicker = new wxColourPickerCtrl(gridlineBox->GetStaticBox(), wxID_ANY);
        gridlineGrid->Add(m_gridlineColorPicker, wxSizerFlags{}.Expand());

        gridlineGrid->Add(new wxStaticText(gridlineBox->GetStaticBox(), wxID_ANY, _(L"Width:")),
                          wxSizerFlags{}.CenterVertical());
        m_gridlineWidthSpin =
            new wxSpinCtrl(gridlineBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, wxSP_ARROW_KEYS, 1, 10, 1);
        gridlineGrid->Add(m_gridlineWidthSpin, wxSizerFlags{}.Expand());

        gridlineGrid->Add(new wxStaticText(gridlineBox->GetStaticBox(), wxID_ANY, _(L"Style:")),
                          wxSizerFlags{}.CenterVertical());
        m_gridlineStyleChoice = new wxChoice(gridlineBox->GetStaticBox(), wxID_ANY);
        m_gridlineStyleChoice->Append(_(L"None"));
        m_gridlineStyleChoice->Append(_(L"Solid"));
        m_gridlineStyleChoice->Append(_(L"Dot"));
        m_gridlineStyleChoice->Append(_(L"Long dash"));
        m_gridlineStyleChoice->Append(_(L"Short dash"));
        m_gridlineStyleChoice->Append(_(L"Dot dash"));
        gridlineGrid->Add(m_gridlineStyleChoice, wxSizerFlags{}.Expand());

        gridSizer->Add(gridlineBox, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND | wxALL,
                       FromDIP(4));

        // Group 4: Tickmarks
        //------------------
        auto* tickBox = new wxStaticBoxSizer(wxVERTICAL, axisPage, _(L"Tickmarks"));
        auto* tickGrid = new wxFlexGridSizer(2, FromDIP(4), FromDIP(4));
        tickGrid->AddGrowableCol(1, 1);
        tickBox->Add(tickGrid, wxSizerFlags{}.Expand().Border());

        tickGrid->Add(new wxStaticText(tickBox->GetStaticBox(), wxID_ANY, _(L"Display:")),
                      wxSizerFlags{}.CenterVertical());
        m_tickmarkDisplayChoice = new wxChoice(tickBox->GetStaticBox(), wxID_ANY);
        m_tickmarkDisplayChoice->Append(_(L"Inner"));
        m_tickmarkDisplayChoice->Append(_(L"Outer"));
        m_tickmarkDisplayChoice->Append(_(L"Crossed"));
        m_tickmarkDisplayChoice->Append(_(L"No display"));
        tickGrid->Add(m_tickmarkDisplayChoice, wxSizerFlags{}.Expand());

        gridSizer->Add(tickBox, wxGBPosition(1, 2), wxDefaultSpan, wxEXPAND | wxALL, FromDIP(4));

        // Group 5: Labels
        //------------------
        auto* labelBox = new wxStaticBoxSizer(wxVERTICAL, axisPage, _(L"Labels"));
        auto* labelGrid = new wxFlexGridSizer(2, FromDIP(4), FromDIP(4));
        labelGrid->AddGrowableCol(1, 1);
        labelBox->Add(labelGrid, wxSizerFlags{}.Expand().Border());

        labelGrid->Add(new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Display:")),
                       wxSizerFlags{}.CenterVertical());
        m_labelDisplayChoice = new wxChoice(labelBox->GetStaticBox(), wxID_ANY);
        m_labelDisplayChoice->Append(_(L"Custom labels or values"));
        m_labelDisplayChoice->Append(_(L"Only custom labels"));
        m_labelDisplayChoice->Append(_(L"Custom labels and values"));
        m_labelDisplayChoice->Append(_(L"No display"));
        m_labelDisplayChoice->Append(_(L"Values"));
        labelGrid->Add(m_labelDisplayChoice, wxSizerFlags{}.Expand());

        labelGrid->Add(new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Number format:")),
                       wxSizerFlags{}.CenterVertical());
        m_numberDisplayChoice = new wxChoice(labelBox->GetStaticBox(), wxID_ANY);
        m_numberDisplayChoice->Append(_(L"Value"));
        m_numberDisplayChoice->Append(_(L"Percentage"));
        m_numberDisplayChoice->Append(_(L"Currency"));
        m_numberDisplayChoice->Append(_(L"Value (simple)"));
        labelGrid->Add(m_numberDisplayChoice, wxSizerFlags{}.Expand());

        labelGrid->Add(new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Orientation:")),
                       wxSizerFlags{}.CenterVertical());
        m_labelOrientationChoice = new wxChoice(labelBox->GetStaticBox(), wxID_ANY);
        m_labelOrientationChoice->Append(_(L"Parallel"));
        m_labelOrientationChoice->Append(_(L"Perpendicular"));
        labelGrid->Add(m_labelOrientationChoice, wxSizerFlags{}.Expand());

        labelGrid->Add(new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Perp. alignment:")),
                       wxSizerFlags{}.CenterVertical());
        m_perpAlignmentChoice = new wxChoice(labelBox->GetStaticBox(), wxID_ANY);
        m_perpAlignmentChoice->Append(_(L"Align with axis line"));
        m_perpAlignmentChoice->Append(_(L"Align with boundary"));
        m_perpAlignmentChoice->Append(_(L"Center on axis line"));
        labelGrid->Add(m_perpAlignmentChoice, wxSizerFlags{}.Expand());

        labelGrid->Add(new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Precision:")),
                       wxSizerFlags{}.CenterVertical());
        m_precisionSpin =
            new wxSpinCtrl(labelBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0);
        labelGrid->Add(m_precisionSpin, wxSizerFlags{}.Expand());

        labelGrid->Add(
            new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"Label line length:")),
            wxSizerFlags{}.CenterVertical());
        m_labelLineLengthSpin =
            new wxSpinCtrl(labelBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, wxSP_ARROW_KEYS, 10, 500, 100);
        labelGrid->Add(m_labelLineLengthSpin, wxSizerFlags{}.Expand());

        m_doubleSidedCheck =
            new wxCheckBox(labelBox->GetStaticBox(), wxID_ANY, _(L"Double-sided labels"));
        labelBox->Add(m_doubleSidedCheck, wxSizerFlags{}.Border());
        m_showOuterLabelsCheck =
            new wxCheckBox(labelBox->GetStaticBox(), wxID_ANY, _(L"Show outer labels"));
        labelBox->Add(m_showOuterLabelsCheck, wxSizerFlags{}.Border());
        m_stackLabelsCheck = new wxCheckBox(labelBox->GetStaticBox(), wxID_ANY, _(L"Stack labels"));
        labelBox->Add(m_stackLabelsCheck, wxSizerFlags{}.Border());

        gridSizer->Add(labelBox, wxGBPosition(0, 1), wxGBSpan(1, 2), wxEXPAND | wxALL, FromDIP(4));

        // Group 6: Brackets
        //------------------
        auto* bracketBox = new wxStaticBoxSizer(wxVERTICAL, axisPage, _(L"Brackets"));
        m_bracketListBox = new wxEditableListBox(
            bracketBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(100) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        RefreshBracketList();
        bracketBox->Add(m_bracketListBox, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

        m_bracketListBox->GetNewButton()->Bind(wxEVT_BUTTON,
                                               [this](wxCommandEvent&) { OnAddBracket(); });
        m_bracketListBox->GetNewButton()->SetBitmapLabel(
            wxGetApp().ReadSvgIcon(L"label.svg", wxSize{ 16, 16 }));
        m_bracketListBox->GetEditButton()->Bind(wxEVT_BUTTON,
                                                [this](wxCommandEvent&) { OnEditBracket(); });
        m_bracketListBox->GetDelButton()->Bind(wxEVT_BUTTON,
                                               [this](wxCommandEvent&) { OnRemoveBracket(); });
        m_bracketListBox->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                               [this](wxListEvent&) { OnEditBracket(); });

        auto* addFromDatasetBtn =
            new wxButton(bracketBox->GetStaticBox(), wxID_ANY, _(L"Add from dataset..."));
        addFromDatasetBtn->Bind(wxEVT_BUTTON,
                                [this](wxCommandEvent&) { OnAddBracketsFromDataset(); });
        bracketBox->Add(addFromDatasetBtn, wxSizerFlags{}.Border());

        gridSizer->Add(bracketBox, wxGBPosition(2, 0), wxGBSpan(1, 3), wxEXPAND | wxALL,
                       FromDIP(4));

        // global mirror checkboxes
        gridSizer->Add(new wxCheckBox(axisPage, wxID_ANY, _(L"Mirror X axis"), wxDefaultPosition,
                                      wxDefaultSize, 0, wxGenericValidator{ &m_mirrorXAxis }),
                       wxGBPosition(3, 0), wxDefaultSpan, wxALL, FromDIP(4));

        gridSizer->Add(new wxCheckBox(axisPage, wxID_ANY, _(L"Mirror Y axis"), wxDefaultPosition,
                                      wxDefaultSize, 0, wxGenericValidator{ &m_mirrorYAxis }),
                       wxGBPosition(3, 1), wxDefaultSpan, wxALL, FromDIP(4));

        axisSizer->Add(gridSizer, wxSizerFlags{ 1 }.Expand());
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
    void InsertGraphDlg::OnShapeModeChanged()
        {
        const bool useCustom = m_customShapeRadio->GetValue();
        m_useCustomShapeScheme = useCustom;
        m_shapeSchemeChoice->Enable(!useCustom);
        m_customShapeListBox->Enable(useCustom);
        }

    //-------------------------------------------
    void InsertGraphDlg::OnAddCustomShape()
        {
        const auto shapeNames = GetPointShapeNames();
        wxSingleChoiceDialog dlg(this, _(L"Select a shape:"), _(L"Add Shape"), shapeNames);
        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto shape = ReportEnumConvert::ConvertIcon(dlg.GetStringSelection());
        if (shape.has_value())
            {
            m_customShapes.push_back(shape.value());
            RefreshCustomShapeList();
            }
        }

    //-------------------------------------------
    void InsertGraphDlg::OnEditCustomShape()
        {
        const auto sel = m_customShapeListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                          wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_customShapes.size()))
            {
            return;
            }

        const auto shapeNames = GetPointShapeNames();

        // preselect the current shape
        const auto currentName =
            ReportEnumConvert::ConvertIconToString(m_customShapes[static_cast<size_t>(sel)]);
        int defaultSel = 0;
        if (currentName.has_value())
            {
            defaultSel = shapeNames.Index(currentName.value());
            if (defaultSel == wxNOT_FOUND)
                {
                defaultSel = 0;
                }
            }

        wxSingleChoiceDialog dlg(this, _(L"Select a shape:"), _(L"Edit Shape"), shapeNames);
        dlg.SetSelection(defaultSel);
        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto shape = ReportEnumConvert::ConvertIcon(dlg.GetStringSelection());
        if (shape.has_value())
            {
            m_customShapes[static_cast<size_t>(sel)] = shape.value();
            RefreshCustomShapeList();
            }
        }

    //-------------------------------------------
    void InsertGraphDlg::OnRemoveCustomShape()
        {
        const auto sel = m_customShapeListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                          wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_customShapes.size()))
            {
            return;
            }

        m_customShapes.erase(m_customShapes.begin() + sel);
        RefreshCustomShapeList();
        }

    //-------------------------------------------
    void InsertGraphDlg::RefreshCustomShapeList()
        {
        wxArrayString strings;
        strings.reserve(m_customShapes.size());
        for (const auto& shape : m_customShapes)
            {
            const auto name = ReportEnumConvert::ConvertIconToString(shape);
            strings.Add(name.value_or(_(L"unknown")));
            }
        m_customShapeListBox->SetStrings(strings);
        }

    //-------------------------------------------
    void InsertGraphDlg::RefreshReferenceLineList()
        {
        wxArrayString strings;
        strings.reserve(m_referenceLines.size());
        for (const auto& rl : m_referenceLines)
            {
            const auto atStr = ReportEnumConvert::ConvertAxisTypeToString(rl.GetAxisType());
            strings.Add(wxString::Format(
                /* TRANSLATORS: value at an axis position on a graph */ _(L"%s at %g: %s"),
                atStr.value_or(L"?"), rl.GetAxisPosition(), rl.GetLabel()));
            }
        m_refLineListBox->SetStrings(strings);
        }

    //-------------------------------------------
    void InsertGraphDlg::OnAddReferenceLine()
        {
        wxDialog dlg(this, wxID_ANY, _(L"Add Reference Line"), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* sizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        sizer->AddGrowableCol(1, 1);

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Axis:")), wxSizerFlags{}.CenterVertical());
        auto* axisChoice = new wxChoice(&dlg, wxID_ANY);
        axisChoice->Append(_(L"Bottom X"));
        axisChoice->Append(_(L"Top X"));
        axisChoice->Append(_(L"Left Y"));
        axisChoice->Append(_(L"Right Y"));
        axisChoice->SetSelection(0);
        sizer->Add(axisChoice, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Position:")),
                   wxSizerFlags{}.CenterVertical());
        auto* posSpin = new wxSpinCtrlDouble(&dlg, wxID_ANY);
        posSpin->SetRange(-1e9, 1e9);
        posSpin->SetIncrement(1);
        posSpin->SetDigits(4);
        sizer->Add(posSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Label:")), wxSizerFlags{}.CenterVertical());
        auto* labelCtrl = new wxTextCtrl(&dlg, wxID_ANY);
        sizer->Add(labelCtrl, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Color:")), wxSizerFlags{}.CenterVertical());
        auto* colorPicker = new wxColourPickerCtrl(
            &dlg, wxID_ANY, Colors::ColorBrewer::GetColor(Colors::Color::LightGray));
        sizer->Add(colorPicker);

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Label placement:")),
                   wxSizerFlags{}.CenterVertical());
        auto* placementChoice = new wxChoice(&dlg, wxID_ANY);
        placementChoice->Append(_(L"Legend"));
        placementChoice->Append(_(L"Opposite axis"));
        placementChoice->SetSelection(0);
        sizer->Add(placementChoice, wxSizerFlags{}.Expand());

        auto* topSizer = new wxBoxSizer(wxVERTICAL);
        topSizer->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
        topSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                      wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(topSizer);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        constexpr AxisType axisTypes[] = { AxisType::BottomXAxis, AxisType::TopXAxis,
                                           AxisType::LeftYAxis, AxisType::RightYAxis };
        const auto axisType = axisTypes[axisChoice->GetSelection()];
        const auto labelPlacement = (placementChoice->GetSelection() == 1) ?
                                        ReferenceLabelPlacement::OppositeAxis :
                                        ReferenceLabelPlacement::Legend;
        m_referenceLines.emplace_back(
            axisType, posSpin->GetValue(), labelCtrl->GetValue(),
            wxPen{ colorPicker->GetColour(), 1, wxPenStyle::wxPENSTYLE_LONG_DASH }, labelPlacement);
        RefreshReferenceLineList();
        }

    //-------------------------------------------
    void InsertGraphDlg::OnEditReferenceLine()
        {
        const auto sel = m_refLineListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                      wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_referenceLines.size()))
            {
            return;
            }

        auto& rl = m_referenceLines[static_cast<size_t>(sel)];

        wxDialog dlg(this, wxID_ANY, _(L"Edit Reference Line"), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* sizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        sizer->AddGrowableCol(1, 1);

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Axis:")), wxSizerFlags{}.CenterVertical());
        auto* axisChoice = new wxChoice(&dlg, wxID_ANY);
        axisChoice->Append(_(L"Bottom X"));
        axisChoice->Append(_(L"Top X"));
        axisChoice->Append(_(L"Left Y"));
        axisChoice->Append(_(L"Right Y"));
        constexpr AxisType axisTypes[] = { AxisType::BottomXAxis, AxisType::TopXAxis,
                                           AxisType::LeftYAxis, AxisType::RightYAxis };
        for (int i = 0; i < 4; ++i)
            {
            if (axisTypes[i] == rl.GetAxisType())
                {
                axisChoice->SetSelection(i);
                break;
                }
            }
        sizer->Add(axisChoice, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Position:")),
                   wxSizerFlags{}.CenterVertical());
        auto* posSpin = new wxSpinCtrlDouble(&dlg, wxID_ANY);
        posSpin->SetRange(-1e9, 1e9);
        posSpin->SetIncrement(1);
        posSpin->SetDigits(4);
        posSpin->SetValue(rl.GetAxisPosition());
        sizer->Add(posSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Label:")), wxSizerFlags{}.CenterVertical());
        auto* labelCtrl = new wxTextCtrl(&dlg, wxID_ANY, rl.GetLabel());
        sizer->Add(labelCtrl, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Color:")), wxSizerFlags{}.CenterVertical());
        auto* colorPicker = new wxColourPickerCtrl(&dlg, wxID_ANY, rl.GetPen().GetColour());
        sizer->Add(colorPicker);

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Label placement:")),
                   wxSizerFlags{}.CenterVertical());
        auto* placementChoice = new wxChoice(&dlg, wxID_ANY);
        placementChoice->Append(_(L"Legend"));
        placementChoice->Append(_(L"Opposite axis"));
        placementChoice->SetSelection(
            rl.GetLabelPlacement() == ReferenceLabelPlacement::OppositeAxis ? 1 : 0);
        sizer->Add(placementChoice, wxSizerFlags{}.Expand());

        auto* topSizer = new wxBoxSizer(wxVERTICAL);
        topSizer->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
        topSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                      wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(topSizer);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto axisType = axisTypes[axisChoice->GetSelection()];
        const auto labelPlacement = (placementChoice->GetSelection() == 1) ?
                                        ReferenceLabelPlacement::OppositeAxis :
                                        ReferenceLabelPlacement::Legend;
        rl = GraphItems::ReferenceLine(
            axisType, posSpin->GetValue(), labelCtrl->GetValue(),
            wxPen{ colorPicker->GetColour(), 1, wxPenStyle::wxPENSTYLE_LONG_DASH }, labelPlacement);
        RefreshReferenceLineList();
        }

    //-------------------------------------------
    void InsertGraphDlg::OnRemoveReferenceLine()
        {
        const auto sel = m_refLineListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                      wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_referenceLines.size()))
            {
            return;
            }

        m_referenceLines.erase(m_referenceLines.begin() + sel);
        RefreshReferenceLineList();
        }

    //-------------------------------------------
    void InsertGraphDlg::RefreshReferenceAreaList()
        {
        wxArrayString strings;
        strings.reserve(m_referenceAreas.size());
        for (const auto& ra : m_referenceAreas)
            {
            const auto atStr = ReportEnumConvert::ConvertAxisTypeToString(ra.GetAxisType());
            strings.Add(wxString::Format(L"%s [%g–%g]: %s", atStr.value_or(L"?"),
                                         ra.GetAxisPosition(), ra.GetAxisPosition2(),
                                         ra.GetLabel()));
            }
        m_refAreaListBox->SetStrings(strings);
        }

    //-------------------------------------------
    void InsertGraphDlg::OnAddReferenceArea()
        {
        wxDialog dlg(this, wxID_ANY, _(L"Add Reference Area"), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* sizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        sizer->AddGrowableCol(1, 1);

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Axis:")), wxSizerFlags{}.CenterVertical());
        auto* axisChoice = new wxChoice(&dlg, wxID_ANY);
        axisChoice->Append(_(L"Bottom X"));
        axisChoice->Append(_(L"Top X"));
        axisChoice->Append(_(L"Left Y"));
        axisChoice->Append(_(L"Right Y"));
        axisChoice->SetSelection(0);
        sizer->Add(axisChoice, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Start:")), wxSizerFlags{}.CenterVertical());
        auto* startSpin = new wxSpinCtrlDouble(&dlg, wxID_ANY);
        startSpin->SetRange(-1e9, 1e9);
        startSpin->SetIncrement(1);
        startSpin->SetDigits(4);
        sizer->Add(startSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"End:")), wxSizerFlags{}.CenterVertical());
        auto* endSpin = new wxSpinCtrlDouble(&dlg, wxID_ANY);
        endSpin->SetRange(-1e9, 1e9);
        endSpin->SetIncrement(1);
        endSpin->SetDigits(4);
        sizer->Add(endSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Label:")), wxSizerFlags{}.CenterVertical());
        auto* labelCtrl = new wxTextCtrl(&dlg, wxID_ANY);
        sizer->Add(labelCtrl, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Color:")), wxSizerFlags{}.CenterVertical());
        auto* colorPicker = new wxColourPickerCtrl(
            &dlg, wxID_ANY, Colors::ColorBrewer::GetColor(Colors::Color::LightGray));
        sizer->Add(colorPicker);

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Style:")), wxSizerFlags{}.CenterVertical());
        auto* styleChoice = new wxChoice(&dlg, wxID_ANY);
        styleChoice->Append(_(L"Solid"));
        styleChoice->Append(_(L"Fade from left to right"));
        styleChoice->Append(_(L"Fade from right to left"));
        styleChoice->SetSelection(0);
        sizer->Add(styleChoice, wxSizerFlags{}.Expand());

        auto* topSizer = new wxBoxSizer(wxVERTICAL);
        topSizer->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
        topSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                      wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(topSizer);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        constexpr AxisType axisTypes[] = { AxisType::BottomXAxis, AxisType::TopXAxis,
                                           AxisType::LeftYAxis, AxisType::RightYAxis };
        constexpr ReferenceAreaStyle areaStyles[] = { ReferenceAreaStyle::Solid,
                                                      ReferenceAreaStyle::FadeFromLeftToRight,
                                                      ReferenceAreaStyle::FadeFromRightToLeft };
        m_referenceAreas.emplace_back(
            axisTypes[axisChoice->GetSelection()], startSpin->GetValue(), endSpin->GetValue(),
            labelCtrl->GetValue(),
            wxPen{ colorPicker->GetColour(), 1, wxPenStyle::wxPENSTYLE_LONG_DASH },
            areaStyles[styleChoice->GetSelection()]);
        RefreshReferenceAreaList();
        }

    //-------------------------------------------
    void InsertGraphDlg::OnEditReferenceArea()
        {
        const auto sel = m_refAreaListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                      wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_referenceAreas.size()))
            {
            return;
            }

        auto& ra = m_referenceAreas[static_cast<size_t>(sel)];

        wxDialog dlg(this, wxID_ANY, _(L"Edit Reference Area"), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* sizer = new wxFlexGridSizer{ 2, wxSize{ FromDIP(8), FromDIP(4) } };
        sizer->AddGrowableCol(1, 1);

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Axis:")), wxSizerFlags{}.CenterVertical());
        auto* axisChoice = new wxChoice(&dlg, wxID_ANY);
        axisChoice->Append(_(L"Bottom X"));
        axisChoice->Append(_(L"Top X"));
        axisChoice->Append(_(L"Left Y"));
        axisChoice->Append(_(L"Right Y"));
        constexpr AxisType axisTypes[] = { AxisType::BottomXAxis, AxisType::TopXAxis,
                                           AxisType::LeftYAxis, AxisType::RightYAxis };
        for (int i = 0; i < 4; ++i)
            {
            if (axisTypes[i] == ra.GetAxisType())
                {
                axisChoice->SetSelection(i);
                break;
                }
            }
        sizer->Add(axisChoice, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Start:")), wxSizerFlags{}.CenterVertical());
        auto* startSpin = new wxSpinCtrlDouble(&dlg, wxID_ANY);
        startSpin->SetRange(-1e9, 1e9);
        startSpin->SetIncrement(1);
        startSpin->SetDigits(4);
        startSpin->SetValue(ra.GetAxisPosition());
        sizer->Add(startSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"End:")), wxSizerFlags{}.CenterVertical());
        auto* endSpin = new wxSpinCtrlDouble(&dlg, wxID_ANY);
        endSpin->SetRange(-1e9, 1e9);
        endSpin->SetIncrement(1);
        endSpin->SetDigits(4);
        endSpin->SetValue(ra.GetAxisPosition2());
        sizer->Add(endSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Label:")), wxSizerFlags{}.CenterVertical());
        auto* labelCtrl = new wxTextCtrl(&dlg, wxID_ANY, ra.GetLabel());
        sizer->Add(labelCtrl, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Color:")), wxSizerFlags{}.CenterVertical());
        auto* colorPicker = new wxColourPickerCtrl(&dlg, wxID_ANY, ra.GetPen().GetColour());
        sizer->Add(colorPicker);

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Style:")), wxSizerFlags{}.CenterVertical());
        auto* styleChoice = new wxChoice(&dlg, wxID_ANY);
        styleChoice->Append(_(L"Solid"));
        styleChoice->Append(_(L"Fade from left to right"));
        styleChoice->Append(_(L"Fade from right to left"));
        constexpr std::array<ReferenceAreaStyle, 3> areaStyles{
            ReferenceAreaStyle::Solid, ReferenceAreaStyle::FadeFromLeftToRight,
            ReferenceAreaStyle::FadeFromRightToLeft
        };
        for (int i = 0; std::cmp_less(i, areaStyles.size()); ++i)
            {
            if (areaStyles[i] == ra.GetReferenceAreaStyle())
                {
                styleChoice->SetSelection(i);
                break;
                }
            }
        sizer->Add(styleChoice, wxSizerFlags{}.Expand());

        auto* topSizer = new wxBoxSizer(wxVERTICAL);
        topSizer->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
        topSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                      wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(topSizer);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        ra = GraphItems::ReferenceArea(
            axisTypes[axisChoice->GetSelection()], startSpin->GetValue(), endSpin->GetValue(),
            labelCtrl->GetValue(),
            wxPen{ colorPicker->GetColour(), 1, wxPenStyle::wxPENSTYLE_LONG_DASH },
            areaStyles[styleChoice->GetSelection()]);
        RefreshReferenceAreaList();
        }

    //-------------------------------------------
    void InsertGraphDlg::OnRemoveReferenceArea()
        {
        const auto sel = m_refAreaListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                      wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_referenceAreas.size()))
            {
            return;
            }

        m_referenceAreas.erase(m_referenceAreas.begin() + sel);
        RefreshReferenceAreaList();
        }

    //-------------------------------------------
    void InsertGraphDlg::RefreshAnnotationList()
        {
        wxArrayString strings;
        strings.reserve(m_annotations.size());
        for (const auto& ann : m_annotations)
            {
            wxString summary = ann.label.GetText().substr(0, 40);
            summary.Replace(L"\n", L" ");
            if (ann.label.GetText().length() > 40)
                {
                summary += L"…";
                }
            // TRANSLATORS: annotation summary shown in the list;
            // %s is the annotation text, %.4g values are X and Y coordinates
            strings.Add(wxString::Format(_(L"\"%s\" at (%.4g, %.4g)"), summary, ann.anchor.m_x,
                                         ann.anchor.m_y));
            }
        m_annotationListBox->SetStrings(strings);
        }

    //-------------------------------------------
    void InsertGraphDlg::OnAddAnnotation()
        {
        wxDialog dlg(this, wxID_ANY, _(L"Add Annotation"), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* topSizer = new wxBoxSizer(wxVERTICAL);

        // label section
        auto* labelBox = new wxStaticBoxSizer(wxHORIZONTAL, &dlg, _(L"Label"));
        auto* labelPreview = new wxStaticText(labelBox->GetStaticBox(), wxID_ANY, _(L"(none)"),
                                              wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
        labelPreview->SetMinSize(FromDIP(wxSize{ 200, -1 }));
        labelBox->Add(labelPreview, wxSizerFlags{ 1 }.CenterVertical().Border());
        GraphItems::Label annotLabel;
        auto* editLabelBtn = new wxButton(labelBox->GetStaticBox(), wxID_ANY, _(L"Edit..."));
        editLabelBtn->Bind(
            wxEVT_BUTTON, [this, &annotLabel, labelPreview](wxCommandEvent&)
            { EditLabelHelper(annotLabel, labelPreview, _(L"Edit Annotation Label")); });
        labelBox->Add(editLabelBtn, wxSizerFlags{}.Border());
        topSizer->Add(labelBox, wxSizerFlags{}.Expand().Border());

        // anchoring
        auto* anchoringBox = new wxStaticBoxSizer(wxHORIZONTAL, &dlg, _(L"Anchoring"));
        anchoringBox->Add(new wxStaticText(anchoringBox->GetStaticBox(), wxID_ANY, _(L"Mode:")),
                          wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        auto* anchoringChoice = new wxChoice(anchoringBox->GetStaticBox(), wxID_ANY);
        anchoringChoice->Append(_(L"Center"));
        anchoringChoice->Append(_(L"Top-left corner"));
        anchoringChoice->Append(_(L"Top-right corner"));
        anchoringChoice->Append(_(L"Bottom-left corner"));
        anchoringChoice->Append(_(L"Bottom-right corner"));
        anchoringChoice->SetSelection(0);
        anchoringBox->Add(anchoringChoice, wxSizerFlags{}.Border());
        topSizer->Add(anchoringBox, wxSizerFlags{}.Expand().Border());

        // anchor point
        auto* anchorBox = new wxStaticBoxSizer(wxVERTICAL, &dlg, _(L"Anchor Point"));
        auto* anchorGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        anchorGrid->AddGrowableCol(1, 1);

        anchorGrid->Add(new wxStaticText(anchorBox->GetStaticBox(), wxID_ANY, _(L"x-axis:")),
                        wxSizerFlags{}.CenterVertical());
        auto* anchorXSpin = new wxSpinCtrlDouble(anchorBox->GetStaticBox(), wxID_ANY);
        anchorXSpin->SetRange(-1e9, 1e9);
        anchorXSpin->SetIncrement(1);
        anchorXSpin->SetDigits(4);
        anchorGrid->Add(anchorXSpin, wxSizerFlags{}.Expand());

        anchorGrid->Add(new wxStaticText(anchorBox->GetStaticBox(), wxID_ANY, _(L"y-axis:")),
                        wxSizerFlags{}.CenterVertical());
        auto* anchorYSpin = new wxSpinCtrlDouble(anchorBox->GetStaticBox(), wxID_ANY);
        anchorYSpin->SetRange(-1e9, 1e9);
        anchorYSpin->SetIncrement(1);
        anchorYSpin->SetDigits(4);
        anchorGrid->Add(anchorYSpin, wxSizerFlags{}.Expand());

        anchorBox->Add(anchorGrid, wxSizerFlags{}.Expand().Border(wxLEFT | wxBOTTOM));
        topSizer->Add(anchorBox, wxSizerFlags{}.Expand().Border());

        // interest points
        auto* ipBox = new wxStaticBoxSizer(wxVERTICAL, &dlg, _(L"Interest Points"));
        auto* ipList = new wxEditableListBox(
            ipBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
            wxSize{ FromDIP(280), FromDIP(100) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        std::vector<wxPoint2DDouble> interestPts;

        const auto refreshIpList = [ipList, &interestPts]()
        {
            wxArrayString strings;
            strings.reserve(interestPts.size());
            for (const auto& pt : interestPts)
                {
                strings.Add(wxString::Format(L"(%.4g, %.4g)", pt.m_x, pt.m_y));
                }
            ipList->SetStrings(strings);
        };

        ipList->GetNewButton()->Bind(
            wxEVT_BUTTON,
            [this, &interestPts, refreshIpList](wxCommandEvent&)
            {
                wxDialog ipDlg(this, wxID_ANY, _(L"Add Interest Point"), wxDefaultPosition,
                               wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
                auto* sizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
                sizer->AddGrowableCol(1, 1);

                sizer->Add(new wxStaticText(&ipDlg, wxID_ANY, _(L"x-axis:")),
                           wxSizerFlags{}.CenterVertical());
                auto* xSpin = new wxSpinCtrlDouble(&ipDlg, wxID_ANY);
                xSpin->SetRange(-1e9, 1e9);
                xSpin->SetIncrement(1);
                xSpin->SetDigits(4);
                sizer->Add(xSpin, wxSizerFlags{}.Expand());

                sizer->Add(new wxStaticText(&ipDlg, wxID_ANY, _(L"y-axis:")),
                           wxSizerFlags{}.CenterVertical());
                auto* ySpin = new wxSpinCtrlDouble(&ipDlg, wxID_ANY);
                ySpin->SetRange(-1e9, 1e9);
                ySpin->SetIncrement(1);
                ySpin->SetDigits(4);
                sizer->Add(ySpin, wxSizerFlags{}.Expand());

                auto* ts = new wxBoxSizer(wxVERTICAL);
                ts->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
                ts->Add(ipDlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                        wxSizerFlags{}.Expand().Border());
                ipDlg.SetSizerAndFit(ts);

                if (ipDlg.ShowModal() == wxID_OK)
                    {
                    interestPts.emplace_back(xSpin->GetValue(), ySpin->GetValue());
                    refreshIpList();
                    }
            });
        ipList->GetEditButton()->Bind(
            wxEVT_BUTTON,
            [this, ipList, &interestPts, refreshIpList](wxCommandEvent&)
            {
                const auto sel =
                    ipList->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, interestPts.size()))
                    {
                    return;
                    }
                auto& pt = interestPts[static_cast<size_t>(sel)];

                wxDialog ipDlg(this, wxID_ANY, _(L"Edit Interest Point"), wxDefaultPosition,
                               wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
                auto* sizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
                sizer->AddGrowableCol(1, 1);

                sizer->Add(new wxStaticText(&ipDlg, wxID_ANY, _(L"x-axis:")),
                           wxSizerFlags{}.CenterVertical());
                auto* xSpin = new wxSpinCtrlDouble(&ipDlg, wxID_ANY);
                xSpin->SetRange(-1e9, 1e9);
                xSpin->SetIncrement(1);
                xSpin->SetDigits(4);
                xSpin->SetValue(pt.m_x);
                sizer->Add(xSpin, wxSizerFlags{}.Expand());

                sizer->Add(new wxStaticText(&ipDlg, wxID_ANY, _(L"y-axis:")),
                           wxSizerFlags{}.CenterVertical());
                auto* ySpin = new wxSpinCtrlDouble(&ipDlg, wxID_ANY);
                ySpin->SetRange(-1e9, 1e9);
                ySpin->SetIncrement(1);
                ySpin->SetDigits(4);
                ySpin->SetValue(pt.m_y);
                sizer->Add(ySpin, wxSizerFlags{}.Expand());

                auto* ts = new wxBoxSizer(wxVERTICAL);
                ts->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
                ts->Add(ipDlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                        wxSizerFlags{}.Expand().Border());
                ipDlg.SetSizerAndFit(ts);

                if (ipDlg.ShowModal() == wxID_OK)
                    {
                    pt.m_x = xSpin->GetValue();
                    pt.m_y = ySpin->GetValue();
                    refreshIpList();
                    }
            });
        ipList->GetDelButton()->Bind(
            wxEVT_BUTTON,
            [ipList, &interestPts, refreshIpList](wxCommandEvent&)
            {
                const auto sel =
                    ipList->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, interestPts.size()))
                    {
                    return;
                    }
                interestPts.erase(interestPts.begin() + sel);
                refreshIpList();
            });
        ipList->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                     [ipList](wxListEvent&)
                     {
                         wxCommandEvent evt(wxEVT_BUTTON);
                         ipList->GetEditButton()->ProcessWindowEvent(evt);
                     });

        ipBox->Add(ipList, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));
        topSizer->Add(ipBox, wxSizerFlags{ 1 }.Expand().Border());

        topSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                      wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(topSizer);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        if (annotLabel.GetText().empty())
            {
            return;
            }

        // apply anchoring selection
        constexpr Anchoring anchorings[] = { Anchoring::Center, Anchoring::TopLeftCorner,
                                             Anchoring::TopRightCorner, Anchoring::BottomLeftCorner,
                                             Anchoring::BottomRightCorner };
        annotLabel.SetAnchoring(anchorings[anchoringChoice->GetSelection()]);

        // apply pen and background if not already set
        if (!annotLabel.GetPen().IsOk())
            {
            annotLabel.GetPen() = wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black) };
            }
        if (!annotLabel.GetFontBackgroundColor().IsOk())
            {
            annotLabel.SetFontBackgroundColor(
                Colors::ColorContrast::BlackOrWhiteContrast(annotLabel.GetFontColor()));
            }
        annotLabel.SetPadding(5, 5, 5, 5);

        m_annotations.push_back(
            { std::move(annotLabel),
              wxPoint2DDouble{ anchorXSpin->GetValue(), anchorYSpin->GetValue() },
              std::move(interestPts) });
        RefreshAnnotationList();
        }

    //-------------------------------------------
    void InsertGraphDlg::OnEditAnnotation()
        {
        const auto sel = m_annotationListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                         wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_annotations.size()))
            {
            return;
            }

        auto& ann = m_annotations[static_cast<size_t>(sel)];

        wxDialog dlg(this, wxID_ANY, _(L"Edit Annotation"), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* topSizer = new wxBoxSizer(wxVERTICAL);

        // label section
        auto* labelBox = new wxStaticBoxSizer(wxHORIZONTAL, &dlg, _(L"Label"));
        auto* labelPreview = new wxStaticText(
            labelBox->GetStaticBox(), wxID_ANY,
            ann.label.GetText().empty() ? _(L"(none)") : ann.label.GetText().substr(0, 60),
            wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END);
        labelPreview->SetMinSize(FromDIP(wxSize{ 200, -1 }));
        labelBox->Add(labelPreview, wxSizerFlags{ 1 }.CenterVertical().Border());
        GraphItems::Label annotLabel = ann.label;
        auto* editLabelBtn = new wxButton(labelBox->GetStaticBox(), wxID_ANY, _(L"Edit..."));
        editLabelBtn->Bind(
            wxEVT_BUTTON, [this, &annotLabel, labelPreview](wxCommandEvent&)
            { EditLabelHelper(annotLabel, labelPreview, _(L"Edit Annotation Label")); });
        labelBox->Add(editLabelBtn, wxSizerFlags{}.Border());
        topSizer->Add(labelBox, wxSizerFlags{}.Expand().Border());

        // anchoring
        auto* anchoringBox = new wxStaticBoxSizer(wxHORIZONTAL, &dlg, _(L"Anchoring"));
        anchoringBox->Add(new wxStaticText(anchoringBox->GetStaticBox(), wxID_ANY, _(L"Mode:")),
                          wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        auto* anchoringChoice = new wxChoice(anchoringBox->GetStaticBox(), wxID_ANY);
        anchoringChoice->Append(_(L"Center"));
        anchoringChoice->Append(_(L"Top-left corner"));
        anchoringChoice->Append(_(L"Top-right corner"));
        anchoringChoice->Append(_(L"Bottom-left corner"));
        anchoringChoice->Append(_(L"Bottom-right corner"));
            // map current anchoring to selection
            {
            int anchorSel = 0;
            switch (ann.label.GetAnchoring())
                {
            case Anchoring::Center:
                anchorSel = 0;
                break;
            case Anchoring::TopLeftCorner:
                anchorSel = 1;
                break;
            case Anchoring::TopRightCorner:
                anchorSel = 2;
                break;
            case Anchoring::BottomLeftCorner:
                anchorSel = 3;
                break;
            case Anchoring::BottomRightCorner:
                anchorSel = 4;
                break;
                }
            anchoringChoice->SetSelection(anchorSel);
            }
        anchoringBox->Add(anchoringChoice, wxSizerFlags{}.Border());
        topSizer->Add(anchoringBox, wxSizerFlags{}.Expand().Border());

        // anchor point
        auto* anchorBox = new wxStaticBoxSizer(wxVERTICAL, &dlg, _(L"Anchor Point"));
        auto* anchorGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        anchorGrid->AddGrowableCol(1, 1);

        anchorGrid->Add(new wxStaticText(anchorBox->GetStaticBox(), wxID_ANY, _(L"x-axis:")),
                        wxSizerFlags{}.CenterVertical());
        auto* anchorXSpin = new wxSpinCtrlDouble(anchorBox->GetStaticBox(), wxID_ANY);
        anchorXSpin->SetRange(-1e9, 1e9);
        anchorXSpin->SetIncrement(1);
        anchorXSpin->SetDigits(4);
        anchorXSpin->SetValue(ann.anchor.m_x);
        anchorGrid->Add(anchorXSpin, wxSizerFlags{}.Expand());

        anchorGrid->Add(new wxStaticText(anchorBox->GetStaticBox(), wxID_ANY, _(L"y-axis:")),
                        wxSizerFlags{}.CenterVertical());
        auto* anchorYSpin = new wxSpinCtrlDouble(anchorBox->GetStaticBox(), wxID_ANY);
        anchorYSpin->SetRange(-1e9, 1e9);
        anchorYSpin->SetIncrement(1);
        anchorYSpin->SetDigits(4);
        anchorYSpin->SetValue(ann.anchor.m_y);
        anchorGrid->Add(anchorYSpin, wxSizerFlags{}.Expand());

        anchorBox->Add(anchorGrid, wxSizerFlags{}.Expand().Border(wxLEFT | wxBOTTOM));
        topSizer->Add(anchorBox, wxSizerFlags{}.Expand().Border());

        // interest points
        auto* ipBox = new wxStaticBoxSizer(wxVERTICAL, &dlg, _(L"Interest Points"));
        auto* ipList = new wxEditableListBox(
            ipBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
            wxSize{ FromDIP(280), FromDIP(100) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        std::vector<wxPoint2DDouble> interestPts = ann.interestPts;

        const auto refreshIpList = [ipList, &interestPts]()
        {
            wxArrayString strings;
            strings.reserve(interestPts.size());
            for (const auto& pt : interestPts)
                {
                strings.Add(wxString::Format(L"(%.4g, %.4g)", pt.m_x, pt.m_y));
                }
            ipList->SetStrings(strings);
        };
        refreshIpList();

        ipList->GetNewButton()->Bind(
            wxEVT_BUTTON,
            [this, &interestPts, refreshIpList](wxCommandEvent&)
            {
                wxDialog ipDlg(this, wxID_ANY, _(L"Add Interest Point"), wxDefaultPosition,
                               wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
                auto* sizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
                sizer->AddGrowableCol(1, 1);

                sizer->Add(new wxStaticText(&ipDlg, wxID_ANY, _(L"x-axis:")),
                           wxSizerFlags{}.CenterVertical());
                auto* xSpin = new wxSpinCtrlDouble(&ipDlg, wxID_ANY);
                xSpin->SetRange(-1e9, 1e9);
                xSpin->SetIncrement(1);
                xSpin->SetDigits(4);
                sizer->Add(xSpin, wxSizerFlags{}.Expand());

                sizer->Add(new wxStaticText(&ipDlg, wxID_ANY, _(L"y-axis:")),
                           wxSizerFlags{}.CenterVertical());
                auto* ySpin = new wxSpinCtrlDouble(&ipDlg, wxID_ANY);
                ySpin->SetRange(-1e9, 1e9);
                ySpin->SetIncrement(1);
                ySpin->SetDigits(4);
                sizer->Add(ySpin, wxSizerFlags{}.Expand());

                auto* ts = new wxBoxSizer(wxVERTICAL);
                ts->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
                ts->Add(ipDlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                        wxSizerFlags{}.Expand().Border());
                ipDlg.SetSizerAndFit(ts);

                if (ipDlg.ShowModal() == wxID_OK)
                    {
                    interestPts.emplace_back(xSpin->GetValue(), ySpin->GetValue());
                    refreshIpList();
                    }
            });
        ipList->GetEditButton()->Bind(
            wxEVT_BUTTON,
            [this, ipList, &interestPts, refreshIpList](wxCommandEvent&)
            {
                const auto selected =
                    ipList->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (selected == wxNOT_FOUND || std::cmp_greater_equal(selected, interestPts.size()))
                    {
                    return;
                    }
                auto& pt = interestPts[static_cast<size_t>(selected)];

                wxDialog ipDlg(this, wxID_ANY, _(L"Edit Interest Point"), wxDefaultPosition,
                               wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
                auto* sizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
                sizer->AddGrowableCol(1, 1);

                sizer->Add(new wxStaticText(&ipDlg, wxID_ANY, _(L"x-axis:")),
                           wxSizerFlags{}.CenterVertical());
                auto* xSpin = new wxSpinCtrlDouble(&ipDlg, wxID_ANY);
                xSpin->SetRange(-1e9, 1e9);
                xSpin->SetIncrement(1);
                xSpin->SetDigits(4);
                xSpin->SetValue(pt.m_x);
                sizer->Add(xSpin, wxSizerFlags{}.Expand());

                sizer->Add(new wxStaticText(&ipDlg, wxID_ANY, _(L"y-axisY:")),
                           wxSizerFlags{}.CenterVertical());
                auto* ySpin = new wxSpinCtrlDouble(&ipDlg, wxID_ANY);
                ySpin->SetRange(-1e9, 1e9);
                ySpin->SetIncrement(1);
                ySpin->SetDigits(4);
                ySpin->SetValue(pt.m_y);
                sizer->Add(ySpin, wxSizerFlags{}.Expand());

                auto* ts = new wxBoxSizer(wxVERTICAL);
                ts->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
                ts->Add(ipDlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                        wxSizerFlags{}.Expand().Border());
                ipDlg.SetSizerAndFit(ts);

                if (ipDlg.ShowModal() == wxID_OK)
                    {
                    pt.m_x = xSpin->GetValue();
                    pt.m_y = ySpin->GetValue();
                    refreshIpList();
                    }
            });
        ipList->GetDelButton()->Bind(
            wxEVT_BUTTON,
            [ipList, &interestPts, refreshIpList](wxCommandEvent&)
            {
                const auto selected =
                    ipList->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (selected == wxNOT_FOUND || std::cmp_greater_equal(selected, interestPts.size()))
                    {
                    return;
                    }
                interestPts.erase(interestPts.begin() + selected);
                refreshIpList();
            });
        ipList->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                     [ipList](wxListEvent&)
                     {
                         wxCommandEvent evt(wxEVT_BUTTON);
                         ipList->GetEditButton()->ProcessWindowEvent(evt);
                     });

        ipBox->Add(ipList, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));
        topSizer->Add(ipBox, wxSizerFlags{ 1 }.Expand().Border());

        topSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                      wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(topSizer);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        if (annotLabel.GetText().empty())
            {
            return;
            }

        // apply anchoring selection
        constexpr Anchoring anchorings[] = { Anchoring::Center, Anchoring::TopLeftCorner,
                                             Anchoring::TopRightCorner, Anchoring::BottomLeftCorner,
                                             Anchoring::BottomRightCorner };
        annotLabel.SetAnchoring(anchorings[anchoringChoice->GetSelection()]);

        // apply pen and background if not already set
        if (!annotLabel.GetPen().IsOk())
            {
            annotLabel.GetPen() = wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black) };
            }
        if (!annotLabel.GetFontBackgroundColor().IsOk())
            {
            annotLabel.SetFontBackgroundColor(
                Colors::ColorContrast::BlackOrWhiteContrast(annotLabel.GetFontColor()));
            }
        annotLabel.SetPadding(5, 5, 5, 5);

        ann.label = std::move(annotLabel);
        ann.anchor = wxPoint2DDouble{ anchorXSpin->GetValue(), anchorYSpin->GetValue() };
        ann.interestPts = std::move(interestPts);
        RefreshAnnotationList();
        }

    //-------------------------------------------
    void InsertGraphDlg::OnRemoveAnnotation()
        {
        const auto sel = m_annotationListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                         wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_annotations.size()))
            {
            return;
            }

        m_annotations.erase(m_annotations.begin() + sel);
        RefreshAnnotationList();
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

        // load annotations
        m_annotations.clear();
        for (const auto& embObj : graph.GetAnnotations())
            {
            const auto* labelPtr = dynamic_cast<const GraphItems::Label*>(embObj.GetObject().get());
            if (labelPtr != nullptr)
                {
                AnnotationInfo info{ *labelPtr, embObj.GetAnchorPoint(),
                                     embObj.GetInterestPoints() };
                // restore the label's original scaling so that
                // AddAnnotation + the graph's recalc don't double-scale
                info.label.SetScaling(embObj.GetOriginalScaling());
                // show the unexpanded template text in the editor
                // so the user sees {{constants}} rather than values
                const auto tmpl = info.label.GetPropertyTemplate(L"text");
                if (!tmpl.empty())
                    {
                    info.label.SetText(tmpl);
                    }
                m_annotations.push_back(std::move(info));
                }
            }
        if (m_annotationListBox != nullptr)
            {
            RefreshAnnotationList();
            }

        // load reference lines and areas
        m_referenceLines = graph.GetReferenceLines();
        m_referenceAreas = graph.GetReferenceAreas();
        if (m_refLineListBox != nullptr)
            {
            RefreshReferenceLineList();
            }
        if (m_refAreaListBox != nullptr)
            {
            RefreshReferenceAreaList();
            }

        // save all four axes for round-tripping (SetData() rebuilds them)
        m_savedAxes.clear();
        for (const auto axisType : { AxisType::BottomXAxis, AxisType::TopXAxis, AxisType::LeftYAxis,
                                     AxisType::RightYAxis })
            {
            m_savedAxes.emplace(axisType, graph.GetAxis(axisType));
            }

        // load the first axis into the per-axis controls
        m_currentAxisType = AxisType::BottomXAxis;
        if (m_axisSelector != nullptr)
            {
            m_axisSelector->SetSelection(0);
            }
        if (m_savedAxes.count(m_currentAxisType) > 0)
            {
            ReadControlsFromAxis(m_savedAxes.at(m_currentAxisType));
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
            const auto& colorScheme = graph.GetColorScheme();
            const auto namedIndex = ColorSchemeToIndex(colorScheme);
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
            else if (namedIndex == 0 && colorScheme != nullptr && !colorScheme->GetColors().empty())
                {
                m_useCustomColors = true;
                m_customColors.clear();
                for (const auto& clr : colorScheme->GetColors())
                    {
                    m_customColors.push_back(clr);
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

        // shape scheme
        if (m_options & GraphDlgIncludeShapeScheme)
            {
            const auto& shapeScheme = graph.GetShapeScheme();
            const auto namedIndex = ShapeSchemeToIndex(shapeScheme);
            if (namedIndex == 0 && shapeScheme != nullptr &&
                !shapeScheme->IsKindOf(wxCLASSINFO(Icons::Schemes::StandardShapes)))
                {
                // custom scheme — copy the shapes into the editable list
                m_useCustomShapeScheme = true;
                m_customShapes = shapeScheme->GetShapes();
                m_shapeSchemeIndex = 0;
                m_customShapeRadio->SetValue(true);
                RefreshCustomShapeList();
                }
            else
                {
                m_useCustomShapeScheme = false;
                m_shapeSchemeIndex = namedIndex;
                m_namedShapeRadio->SetValue(true);
                }
            OnShapeModeChanged();
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
    void InsertGraphDlg::ApplyAxisOverrides(Graphs::Graph2D& graph)
        {
        // flush the currently displayed axis controls into m_savedAxes
        if (m_savedAxes.count(m_currentAxisType) > 0)
            {
            WriteControlsToAxis(m_savedAxes.at(m_currentAxisType));
            }

        for (const auto& [axisType, saved] : m_savedAxes)
            {
            graph.GetAxis(axisType) = saved;
            }
        }

    //-------------------------------------------
    namespace
        {
        int PenStyleToIndex(const wxPenStyle style)
            {
            switch (style)
                {
            case wxPENSTYLE_SOLID:
                return 1;
            case wxPENSTYLE_DOT:
                return 2;
            case wxPENSTYLE_LONG_DASH:
                return 3;
            case wxPENSTYLE_SHORT_DASH:
                return 4;
            case wxPENSTYLE_DOT_DASH:
                return 5;
            case wxPENSTYLE_TRANSPARENT:
                [[fallthrough]];
            default:
                return 0;
                }
            }

        wxPenStyle IndexToPenStyle(const int index)
            {
            constexpr wxPenStyle styles[] = { wxPENSTYLE_TRANSPARENT, wxPENSTYLE_SOLID,
                                              wxPENSTYLE_DOT,        wxPENSTYLE_LONG_DASH,
                                              wxPENSTYLE_SHORT_DASH, wxPENSTYLE_DOT_DASH };
            return (index >= 0 && index < 6) ? styles[index] : wxPENSTYLE_TRANSPARENT;
            }
        } // unnamed namespace

    //-------------------------------------------
    void InsertGraphDlg::OnAxisSelectionChanged()
        {
        // write current controls back to the axis we're leaving
        if (m_savedAxes.count(m_currentAxisType) > 0)
            {
            WriteControlsToAxis(m_savedAxes.at(m_currentAxisType));
            }

        // switch to the new axis
        constexpr AxisType axisTypes[] = { AxisType::BottomXAxis, AxisType::TopXAxis,
                                           AxisType::LeftYAxis, AxisType::RightYAxis };
        const int sel = m_axisSelector->GetSelection();
        if (sel >= 0 && sel < 4)
            {
            m_currentAxisType = axisTypes[sel];
            }

        // load the new axis into controls
        if (m_savedAxes.count(m_currentAxisType) > 0)
            {
            ReadControlsFromAxis(m_savedAxes.at(m_currentAxisType));
            }
        }

    //-------------------------------------------
    void InsertGraphDlg::ReadControlsFromAxis(const GraphItems::Axis& axis)
        {
        // axis line
        if (m_axisLineColorPicker != nullptr)
            {
            const auto& pen = axis.GetAxisLinePen();
            if (pen.IsOk())
                {
                m_axisLineColorPicker->SetColour(pen.GetColour());
                m_axisLineWidthSpin->SetValue(pen.GetWidth());
                m_axisLineStyleChoice->SetSelection(PenStyleToIndex(pen.GetStyle()));
                }
            else
                {
                m_axisLineStyleChoice->SetSelection(0);
                }
            }
        if (m_axisCapStyleChoice != nullptr)
            {
            m_axisCapStyleChoice->SetSelection(
                axis.GetCapStyle() == AxisCapStyle::Arrow ? 1 : 0);
            }
        if (m_axisReverseCheck != nullptr)
            {
            m_axisReverseCheck->SetValue(axis.IsReversed());
            }

        // gridlines
        if (m_gridlineColorPicker != nullptr)
            {
            const auto& pen = axis.GetGridlinePen();
            if (pen.IsOk())
                {
                m_gridlineColorPicker->SetColour(pen.GetColour());
                m_gridlineWidthSpin->SetValue(pen.GetWidth());
                m_gridlineStyleChoice->SetSelection(PenStyleToIndex(pen.GetStyle()));
                }
            else
                {
                m_gridlineStyleChoice->SetSelection(0);
                }
            }

        // tickmarks
        if (m_tickmarkDisplayChoice != nullptr)
            {
            m_tickmarkDisplayChoice->SetSelection(
                static_cast<int>(axis.GetTickMarkDisplay()));
            }

        // labels
        if (m_labelDisplayChoice != nullptr)
            {
            m_labelDisplayChoice->SetSelection(
                static_cast<int>(axis.GetLabelDisplay()));
            }
        if (m_numberDisplayChoice != nullptr)
            {
            m_numberDisplayChoice->SetSelection(
                static_cast<int>(axis.GetNumberDisplay()));
            }
        if (m_labelOrientationChoice != nullptr)
            {
            m_labelOrientationChoice->SetSelection(
                static_cast<int>(axis.GetAxisLabelOrientation()));
            }
        if (m_perpAlignmentChoice != nullptr)
            {
            m_perpAlignmentChoice->SetSelection(
                static_cast<int>(axis.GetPerpendicularLabelAxisAlignment()));
            }
        if (m_precisionSpin != nullptr)
            {
            m_precisionSpin->SetValue(axis.GetPrecision());
            }
        if (m_doubleSidedCheck != nullptr)
            {
            m_doubleSidedCheck->SetValue(axis.HasDoubleSidedAxisLabels());
            }
        if (m_showOuterLabelsCheck != nullptr)
            {
            m_showOuterLabelsCheck->SetValue(axis.IsShowingOuterLabels());
            }
        if (m_stackLabelsCheck != nullptr)
            {
            m_stackLabelsCheck->SetValue(axis.IsStackingLabels());
            }
        if (m_labelLineLengthSpin != nullptr)
            {
            m_labelLineLengthSpin->SetValue(
                static_cast<int>(axis.GetLabelLineLength()));
            }

        // brackets
        RefreshBracketList();
        }

    //-------------------------------------------
    void InsertGraphDlg::WriteControlsToAxis(GraphItems::Axis& axis)
        {
        // axis line
        if (m_axisLineColorPicker != nullptr)
            {
            axis.GetAxisLinePen().SetColour(m_axisLineColorPicker->GetColour());
            axis.GetAxisLinePen().SetWidth(m_axisLineWidthSpin->GetValue());
            axis.GetAxisLinePen().SetStyle(
                IndexToPenStyle(m_axisLineStyleChoice->GetSelection()));
            }
        if (m_axisCapStyleChoice != nullptr)
            {
            axis.SetCapStyle(m_axisCapStyleChoice->GetSelection() == 1 ?
                                 AxisCapStyle::Arrow :
                                 AxisCapStyle::NoCap);
            }
        if (m_axisReverseCheck != nullptr)
            {
            axis.Reverse(m_axisReverseCheck->GetValue());
            }

        // gridlines
        if (m_gridlineColorPicker != nullptr)
            {
            axis.GetGridlinePen().SetColour(m_gridlineColorPicker->GetColour());
            axis.GetGridlinePen().SetWidth(m_gridlineWidthSpin->GetValue());
            axis.GetGridlinePen().SetStyle(
                IndexToPenStyle(m_gridlineStyleChoice->GetSelection()));
            }

        // tickmarks
        if (m_tickmarkDisplayChoice != nullptr)
            {
            const int sel = m_tickmarkDisplayChoice->GetSelection();
            if (sel != wxNOT_FOUND)
                {
                axis.SetTickMarkDisplay(
                    static_cast<GraphItems::Axis::TickMark::DisplayType>(sel));
                }
            }

        // labels
        if (m_labelDisplayChoice != nullptr)
            {
            const int sel = m_labelDisplayChoice->GetSelection();
            if (sel != wxNOT_FOUND)
                {
                axis.SetLabelDisplay(static_cast<AxisLabelDisplay>(sel));
                }
            }
        if (m_numberDisplayChoice != nullptr)
            {
            const int sel = m_numberDisplayChoice->GetSelection();
            if (sel != wxNOT_FOUND)
                {
                axis.SetNumberDisplay(static_cast<NumberDisplay>(sel));
                }
            }
        if (m_labelOrientationChoice != nullptr)
            {
            const int sel = m_labelOrientationChoice->GetSelection();
            if (sel != wxNOT_FOUND)
                {
                axis.SetAxisLabelOrientation(
                    static_cast<AxisLabelOrientation>(sel));
                }
            }
        if (m_perpAlignmentChoice != nullptr)
            {
            const int sel = m_perpAlignmentChoice->GetSelection();
            if (sel != wxNOT_FOUND)
                {
                axis.SetPerpendicularLabelAxisAlignment(
                    static_cast<AxisLabelAlignment>(sel));
                }
            }
        if (m_precisionSpin != nullptr)
            {
            axis.SetPrecision(
                static_cast<uint8_t>(m_precisionSpin->GetValue()));
            }
        if (m_doubleSidedCheck != nullptr)
            {
            axis.SetDoubleSidedAxisLabels(m_doubleSidedCheck->GetValue());
            }
        if (m_showOuterLabelsCheck != nullptr)
            {
            axis.ShowOuterLabels(m_showOuterLabelsCheck->GetValue());
            }
        if (m_stackLabelsCheck != nullptr)
            {
            axis.StackLabels(m_stackLabelsCheck->GetValue());
            }
        if (m_labelLineLengthSpin != nullptr)
            {
            axis.SetLabelLineLength(
                static_cast<size_t>(m_labelLineLengthSpin->GetValue()));
            }
        }

    //-------------------------------------------
    void InsertGraphDlg::RefreshBracketList()
        {
        if (m_bracketListBox == nullptr)
            {
            return;
            }
        wxArrayString items;
        if (m_savedAxes.count(m_currentAxisType) > 0)
            {
            for (const auto& bracket : m_savedAxes.at(m_currentAxisType).GetBrackets())
                {
                items.Add(wxString::Format(L"%s @ %g\u2013%g",
                                           bracket.GetLabel().GetText(),
                                           bracket.GetStartPosition(),
                                           bracket.GetEndPosition()));
                }
            }
        m_bracketListBox->SetStrings(items);
        }

    //-------------------------------------------
    void InsertGraphDlg::OnAddBracket()
        {
        if (m_savedAxes.count(m_currentAxisType) == 0)
            {
            return;
            }
        auto& axis = m_savedAxes.at(m_currentAxisType);

        wxDialog dlg(this, wxID_ANY, _(L"Add Bracket"), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* sizer = new wxFlexGridSizer(2, FromDIP(4), FromDIP(4));
        sizer->AddGrowableCol(1, 1);

        // label combo filled with custom axis labels
        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Label:")),
                   wxSizerFlags{}.CenterVertical());
        auto* labelCombo = new wxComboBox(&dlg, wxID_ANY);
        const auto& customLabels = axis.GetCustomLabels();
        for (const auto& [value, label] : customLabels)
            {
            if (!label.GetText().empty())
                {
                labelCombo->Append(label.GetText(),
                                   new wxStringClientData(wxString::Format(L"%g", value)));
                }
            }
        // also add generated axis point labels
        for (const auto& point : std::as_const(axis).GetAxisPoints())
            {
            if (point.IsShown() && !point.GetDisplayValue().empty())
                {
                labelCombo->Append(point.GetDisplayValue(),
                                   new wxStringClientData(
                                       wxString::Format(L"%g", point.GetValue())));
                }
            }
        sizer->Add(labelCombo, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Start:")),
                   wxSizerFlags{}.CenterVertical());
        auto* startSpin = new wxSpinCtrlDouble(&dlg, wxID_ANY, wxString{}, wxDefaultPosition,
                                               wxDefaultSize, wxSP_ARROW_KEYS, -1e9, 1e9, 0, 0.1);
        sizer->Add(startSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"End:")),
                   wxSizerFlags{}.CenterVertical());
        auto* endSpin = new wxSpinCtrlDouble(&dlg, wxID_ANY, wxString{}, wxDefaultPosition,
                                             wxDefaultSize, wxSP_ARROW_KEYS, -1e9, 1e9, 0, 0.1);
        sizer->Add(endSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Color:")),
                   wxSizerFlags{}.CenterVertical());
        auto* colorPicker = new wxColourPickerCtrl(&dlg, wxID_ANY, *wxBLACK);
        sizer->Add(colorPicker, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Width:")),
                   wxSizerFlags{}.CenterVertical());
        auto* widthSpin = new wxSpinCtrl(&dlg, wxID_ANY, wxString{}, wxDefaultPosition,
                                         wxDefaultSize, wxSP_ARROW_KEYS, 1, 5, 2);
        sizer->Add(widthSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Style:")),
                   wxSizerFlags{}.CenterVertical());
        auto* styleChoice = new wxChoice(&dlg, wxID_ANY);
        styleChoice->Append(_(L"Lines"));
        styleChoice->Append(_(L"Arrow"));
        styleChoice->Append(_(L"Reverse arrow"));
        styleChoice->Append(_(L"No connection lines"));
        styleChoice->Append(_(L"Curly braces"));
        styleChoice->SetSelection(4);
        sizer->Add(styleChoice, wxSizerFlags{}.Expand());

        // auto-fill start/end when a label is selected
        labelCombo->Bind(wxEVT_COMBOBOX, [labelCombo, startSpin, endSpin](wxCommandEvent&)
            {
            const int sel = labelCombo->GetSelection();
            if (sel != wxNOT_FOUND)
                {
                const auto* data =
                    dynamic_cast<wxStringClientData*>(labelCombo->GetClientObject(sel));
                if (data != nullptr)
                    {
                    double val = 0;
                    if (data->GetData().ToDouble(&val))
                        {
                        startSpin->SetValue(val);
                        endSpin->SetValue(val);
                        }
                    }
                }
            });

        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
        mainSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(mainSizer);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const wxString labelText = labelCombo->GetValue();
        if (labelText.empty())
            {
            return;
            }

        constexpr BracketLineStyle bracketStyles[] = {
            BracketLineStyle::Lines, BracketLineStyle::Arrow,
            BracketLineStyle::ReverseArrow, BracketLineStyle::NoConnectionLines,
            BracketLineStyle::CurlyBraces
            };
        const auto bracketStyle =
            (styleChoice->GetSelection() >= 0 && styleChoice->GetSelection() < 5) ?
                bracketStyles[styleChoice->GetSelection()] :
                BracketLineStyle::CurlyBraces;

        const double startPos = startSpin->GetValue();
        const double endPos = endSpin->GetValue();
        const double labelPos = (startPos + endPos) / 2.0;

        GraphItems::Axis::AxisBracket bracket(
            startPos, endPos, labelPos, labelText,
            wxPen(colorPicker->GetColour(), widthSpin->GetValue()),
            bracketStyle);
        axis.AddBracket(std::move(bracket));
        RefreshBracketList();
        }

    //-------------------------------------------
    void InsertGraphDlg::OnEditBracket()
        {
        if (m_savedAxes.count(m_currentAxisType) == 0 || m_bracketListBox == nullptr)
            {
            return;
            }
        auto& axis = m_savedAxes.at(m_currentAxisType);
        auto& brackets = axis.GetBrackets();

        const long sel = m_bracketListBox->GetListCtrl()->GetNextItem(
            -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, brackets.size()))
            {
            return;
            }
        auto& bracket = brackets[sel];

        wxDialog dlg(this, wxID_ANY, _(L"Edit Bracket"), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* sizer = new wxFlexGridSizer(2, FromDIP(4), FromDIP(4));
        sizer->AddGrowableCol(1, 1);

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Label:")),
                   wxSizerFlags{}.CenterVertical());
        auto* labelCtrl = new wxTextCtrl(&dlg, wxID_ANY, bracket.GetLabel().GetText());
        sizer->Add(labelCtrl, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Start:")),
                   wxSizerFlags{}.CenterVertical());
        auto* startSpin = new wxSpinCtrlDouble(&dlg, wxID_ANY, wxString{}, wxDefaultPosition,
                                               wxDefaultSize, wxSP_ARROW_KEYS, -1e9, 1e9,
                                               bracket.GetStartPosition(), 0.1);
        sizer->Add(startSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"End:")),
                   wxSizerFlags{}.CenterVertical());
        auto* endSpin = new wxSpinCtrlDouble(&dlg, wxID_ANY, wxString{}, wxDefaultPosition,
                                             wxDefaultSize, wxSP_ARROW_KEYS, -1e9, 1e9,
                                             bracket.GetEndPosition(), 0.1);
        sizer->Add(endSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Color:")),
                   wxSizerFlags{}.CenterVertical());
        auto* colorPicker =
            new wxColourPickerCtrl(&dlg, wxID_ANY, bracket.GetLinePen().GetColour());
        sizer->Add(colorPicker, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Width:")),
                   wxSizerFlags{}.CenterVertical());
        auto* widthSpin = new wxSpinCtrl(&dlg, wxID_ANY, wxString{}, wxDefaultPosition,
                                         wxDefaultSize, wxSP_ARROW_KEYS, 1, 5,
                                         bracket.GetLinePen().GetWidth());
        sizer->Add(widthSpin, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Style:")),
                   wxSizerFlags{}.CenterVertical());
        auto* styleChoice = new wxChoice(&dlg, wxID_ANY);
        styleChoice->Append(_(L"Lines"));
        styleChoice->Append(_(L"Arrow"));
        styleChoice->Append(_(L"Reverse arrow"));
        styleChoice->Append(_(L"No connection lines"));
        styleChoice->Append(_(L"Curly braces"));
        const int currentStyle = [&bracket]()
            {
            switch (bracket.GetBracketLineStyle())
                {
            case BracketLineStyle::Lines:
                return 0;
            case BracketLineStyle::Arrow:
                return 1;
            case BracketLineStyle::ReverseArrow:
                return 2;
            case BracketLineStyle::NoConnectionLines:
                return 3;
            case BracketLineStyle::CurlyBraces:
                [[fallthrough]];
            default:
                return 4;
                }
            }();
        styleChoice->SetSelection(currentStyle);
        sizer->Add(styleChoice, wxSizerFlags{}.Expand());

        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
        mainSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(mainSizer);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        constexpr BracketLineStyle bracketStyles[] = {
            BracketLineStyle::Lines, BracketLineStyle::Arrow,
            BracketLineStyle::ReverseArrow, BracketLineStyle::NoConnectionLines,
            BracketLineStyle::CurlyBraces
            };
        const auto bracketStyle =
            (styleChoice->GetSelection() >= 0 && styleChoice->GetSelection() < 5) ?
                bracketStyles[styleChoice->GetSelection()] :
                BracketLineStyle::CurlyBraces;

        const double startPos = startSpin->GetValue();
        const double endPos = endSpin->GetValue();
        const double labelPos = (startPos + endPos) / 2.0;

        // replace the bracket
        brackets[sel] = GraphItems::Axis::AxisBracket(
            startPos, endPos, labelPos, labelCtrl->GetValue(),
            wxPen(colorPicker->GetColour(), widthSpin->GetValue()),
            bracketStyle);
        RefreshBracketList();
        }

    //-------------------------------------------
    void InsertGraphDlg::OnRemoveBracket()
        {
        if (m_savedAxes.count(m_currentAxisType) == 0 || m_bracketListBox == nullptr)
            {
            return;
            }
        auto& brackets = m_savedAxes.at(m_currentAxisType).GetBrackets();

        const long sel = m_bracketListBox->GetListCtrl()->GetNextItem(
            -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, brackets.size()))
            {
            return;
            }
        brackets.erase(brackets.begin() + sel);
        RefreshBracketList();
        }

    //-------------------------------------------
    void InsertGraphDlg::OnAddBracketsFromDataset()
        {
        if (m_savedAxes.count(m_currentAxisType) == 0 || GetReportBuilder() == nullptr)
            {
            return;
            }
        auto& axis = m_savedAxes.at(m_currentAxisType);
        const auto& datasets = GetReportBuilder()->GetDatasets();
        if (datasets.empty())
            {
            wxMessageBox(_(L"No datasets are available."), _(L"No Datasets"),
                         wxOK | wxICON_WARNING, this);
            return;
            }

        wxDialog dlg(this, wxID_ANY, _(L"Add Brackets from Dataset"), wxDefaultPosition,
                     wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        auto* sizer = new wxFlexGridSizer(2, FromDIP(4), FromDIP(4));
        sizer->AddGrowableCol(1, 1);

        // dataset choice
        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Dataset:")),
                   wxSizerFlags{}.CenterVertical());
        auto* datasetChoice = new wxChoice(&dlg, wxID_ANY);
        wxArrayString datasetNames;
        for (const auto& [name, dataset] : datasets)
            {
            datasetNames.Add(name);
            datasetChoice->Append(name);
            }
        datasetChoice->SetSelection(0);
        sizer->Add(datasetChoice, wxSizerFlags{}.Expand());

        // label column and value column choices
        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Label column:")),
                   wxSizerFlags{}.CenterVertical());
        auto* labelColChoice = new wxChoice(&dlg, wxID_ANY);
        sizer->Add(labelColChoice, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Value column:")),
                   wxSizerFlags{}.CenterVertical());
        auto* valueColChoice = new wxChoice(&dlg, wxID_ANY);
        sizer->Add(valueColChoice, wxSizerFlags{}.Expand());

        // populate columns from the first dataset
        const auto populateColumns =
            [&datasets, &datasetNames, labelColChoice, valueColChoice, this](const int sel)
            {
            labelColChoice->Clear();
            valueColChoice->Clear();
            if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, datasetNames.size()))
                {
                return;
                }
            const auto it = datasets.find(datasetNames[sel]);
            if (it == datasets.cend() || it->second == nullptr)
                {
                return;
                }
            const auto& ds = *(it->second);
            for (const auto& col : ds.GetCategoricalColumnNames())
                {
                labelColChoice->Append(col);
                valueColChoice->Append(col);
                }
            for (const auto& col : ds.GetContinuousColumnNames())
                {
                valueColChoice->Append(col);
                }
            for (const auto& col : ds.GetDateColumnNames())
                {
                valueColChoice->Append(col);
                }

            // apply hints
            const auto labelHint = GetBracketLabelColumnHint();
            if (!labelHint.empty())
                {
                const int idx = labelColChoice->FindString(labelHint);
                if (idx != wxNOT_FOUND)
                    {
                    labelColChoice->SetSelection(idx);
                    }
                }
            else if (labelColChoice->GetCount() > 0)
                {
                labelColChoice->SetSelection(0);
                }

            const auto valueHint = GetBracketValueColumnHint();
            if (!valueHint.empty())
                {
                const int idx = valueColChoice->FindString(valueHint);
                if (idx != wxNOT_FOUND)
                    {
                    valueColChoice->SetSelection(idx);
                    }
                }
            else if (valueColChoice->GetCount() > 0)
                {
                valueColChoice->SetSelection(0);
                }
            };

        populateColumns(0);
        datasetChoice->Bind(wxEVT_CHOICE,
            [&populateColumns, datasetChoice](wxCommandEvent&)
            { populateColumns(datasetChoice->GetSelection()); });

        // style options
        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Style:")),
                   wxSizerFlags{}.CenterVertical());
        auto* styleChoice = new wxChoice(&dlg, wxID_ANY);
        styleChoice->Append(_(L"Lines"));
        styleChoice->Append(_(L"Arrow"));
        styleChoice->Append(_(L"Reverse arrow"));
        styleChoice->Append(_(L"No connection lines"));
        styleChoice->Append(_(L"Curly braces"));
        styleChoice->SetSelection(4);
        sizer->Add(styleChoice, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Color:")),
                   wxSizerFlags{}.CenterVertical());
        auto* colorPicker = new wxColourPickerCtrl(&dlg, wxID_ANY, *wxBLACK);
        sizer->Add(colorPicker, wxSizerFlags{}.Expand());

        sizer->Add(new wxStaticText(&dlg, wxID_ANY, _(L"Width:")),
                   wxSizerFlags{}.CenterVertical());
        auto* widthSpin = new wxSpinCtrl(&dlg, wxID_ANY, wxString{}, wxDefaultPosition,
                                         wxDefaultSize, wxSP_ARROW_KEYS, 1, 5, 2);
        sizer->Add(widthSpin, wxSizerFlags{}.Expand());

        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->Add(sizer, wxSizerFlags{ 1 }.Expand().Border());
        mainSizer->Add(dlg.CreateStdDialogButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());
        dlg.SetSizerAndFit(mainSizer);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        // get the dataset
        const int dsSel = datasetChoice->GetSelection();
        if (dsSel == wxNOT_FOUND || std::cmp_greater_equal(dsSel, datasetNames.size()))
            {
            return;
            }
        const auto dsIt = datasets.find(datasetNames[dsSel]);
        if (dsIt == datasets.cend() || dsIt->second == nullptr)
            {
            return;
            }

        const wxString labelCol = labelColChoice->GetStringSelection();
        const wxString valueCol = valueColChoice->GetStringSelection();
        if (labelCol.empty() || valueCol.empty())
            {
            return;
            }

        const size_t oldCount = axis.GetBrackets().size();
        axis.AddBrackets(dsIt->second, labelCol, valueCol);

        // apply style and pen to newly added brackets
        constexpr BracketLineStyle bracketStyles[] = {
            BracketLineStyle::Lines, BracketLineStyle::Arrow,
            BracketLineStyle::ReverseArrow, BracketLineStyle::NoConnectionLines,
            BracketLineStyle::CurlyBraces
            };
        const auto bracketStyle =
            (styleChoice->GetSelection() >= 0 && styleChoice->GetSelection() < 5) ?
                bracketStyles[styleChoice->GetSelection()] :
                BracketLineStyle::CurlyBraces;
        const wxPen pen(colorPicker->GetColour(), widthSpin->GetValue());
        for (size_t i = oldCount; i < axis.GetBrackets().size(); ++i)
            {
            axis.GetBrackets()[i].SetBracketLineStyle(bracketStyle);
            axis.GetBrackets()[i].GetLinePen() = pen;
            }

        RefreshBracketList();
        }

    //-------------------------------------------
    wxArrayString InsertGraphDlg::GetShapeSchemeNames()
        {
        wxArrayString names;
        names.Add(_(L"Standard Shapes"));
        names.Add(_(L"Semesters"));
        return names;
        }

    //-------------------------------------------
    std::shared_ptr<Icons::Schemes::IconScheme>
    InsertGraphDlg::ShapeSchemeFromIndex(const int index)
        {
        switch (index)
            {
        case 1:
            return std::make_shared<Icons::Schemes::Semesters>();
        case 0:
            [[fallthrough]];
        default:
            return std::make_shared<Icons::Schemes::StandardShapes>();
            }
        }

    //-------------------------------------------
    int InsertGraphDlg::ShapeSchemeToIndex(
        const std::shared_ptr<Icons::Schemes::IconScheme>& scheme)
        {
        if (scheme == nullptr)
            {
            return 0;
            }
        if (scheme->IsKindOf(wxCLASSINFO(Icons::Schemes::Semesters)))
            {
            return 1;
            }
        return 0;
        }

    //-------------------------------------------
    bool InsertGraphDlg::ValidateShapeScheme()
        {
        if (!(m_options & GraphDlgIncludeShapeScheme))
            {
            return true;
            }
        if (m_useCustomShapeScheme && m_customShapes.empty())
            {
            wxMessageBox(_(L"Please add at least one shape."),
                         _(L"No Shapes Specified"),
                         wxOK | wxICON_WARNING, this);
            return false;
            }
        return true;
        }

    //-------------------------------------------
    wxArrayString InsertGraphDlg::GetPointShapeNames()
        {
        const auto allNames = ReportEnumConvert::GetIconNames();
        wxArrayString names;
        names.reserve(allNames.size());
        for (const auto& name : allNames)
            {
            names.Add(name);
            }
        return names;
        }

    // clang-format on
    } // namespace Wisteria::UI
