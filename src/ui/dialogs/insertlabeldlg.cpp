///////////////////////////////////////////////////////////////////////////////
// Name:        insertlabeldlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertlabeldlg.h"
#include "../../app/wisteriaapp.h"
#include "../../base/reportenumconvert.h"
#include "insertshapedlg.h"
#include <utility>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertLabelDlg::InsertLabelDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                   wxWindow* parent, const wxString& caption, const wxWindowID id,
                                   const wxPoint& pos, const wxSize& size, const long style,
                                   EditMode editMode, const bool includePageOptions)
        : InsertItemDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode),
          m_includePageOptions(includePageOptions)
        {
        SetFitRowToContent(true);
        CreateControls();
        FinalizeControls();
        TransferDataToWindow();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertLabelDlg::CreateControls()
        {
        InsertItemDlg::CreateControls();

        auto* labelPage = new wxPanel(GetSideBarBook());
        auto* mainSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* col1Sizer = new wxBoxSizer(wxVERTICAL);
        auto* col2Sizer = new wxBoxSizer(wxVERTICAL);

        mainSizer->Add(col1Sizer, wxSizerFlags{ 1 }.Expand().Border());
        mainSizer->Add(col2Sizer, wxSizerFlags{ 1 }.Expand().Border());
        labelPage->SetSizer(mainSizer);
        GetSideBarBook()->AddPage(labelPage, _(L"Label"), ID_LABEL_SECTION, true);

        if (!m_includePageOptions)
            {
            GetSideBarBook()->DeletePage(0);
            }

        // first column
        //-------------

        // text
        auto* textSizer = new wxBoxSizer(wxVERTICAL);
        textSizer->Add(new wxStaticText(labelPage, wxID_ANY, _(L"Text:")),
                       wxSizerFlags{}.Border(wxLEFT | wxTOP));
        m_textCtrl = new wxTextCtrl(labelPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                    wxSize{ FromDIP(300), FromDIP(100) }, wxTE_MULTILINE);
        textSizer->Add(m_textCtrl, wxSizerFlags{ 1 }.Expand().Border());
        col1Sizer->Add(textSizer, wxSizerFlags{}.Expand().Border());

        // font options
        auto* fontBox = new wxStaticBoxSizer(wxVERTICAL, labelPage, _(L"Font"));
        auto* fontGrid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(4));
        fontGrid->AddGrowableCol(1, 1);

        fontGrid->Add(new wxStaticText(fontBox->GetStaticBox(), wxID_ANY, _(L"Font:")),
                      wxSizerFlags{}.CenterVertical());
        m_fontPicker = new wxFontPickerCtrl(fontBox->GetStaticBox(), wxID_ANY,
                                            wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
        fontGrid->Add(m_fontPicker, wxSizerFlags{}.Expand());

        fontGrid->Add(new wxStaticText(fontBox->GetStaticBox(), wxID_ANY, _(L"Font color:")),
                      wxSizerFlags{}.CenterVertical());
        m_fontColorPicker = new wxColourPickerCtrl(fontBox->GetStaticBox(), wxID_ANY, *wxBLACK);
        fontGrid->Add(m_fontColorPicker, wxSizerFlags{}.Expand());

        auto* enableBackgroundColorCheck = new wxCheckBox(
            fontBox->GetStaticBox(), wxID_ANY, _(L"Include background color:"), wxDefaultPosition,
            wxDefaultSize, 0, wxGenericValidator{ &m_includeBackgroundColor });
        fontGrid->Add(enableBackgroundColorCheck, wxSizerFlags{}.CenterVertical());
        m_bgColorPicker =
            new wxColourPickerCtrl(fontBox->GetStaticBox(), wxID_ANY, wxTransparentColour);
        fontGrid->Add(m_bgColorPicker, wxSizerFlags{}.Expand());

        fontGrid->Add(new wxStaticText(fontBox->GetStaticBox(), wxID_ANY, _(L"Alignment:")),
                      wxSizerFlags{}.CenterVertical());
            {
            auto* alignChoice =
                new wxChoice(fontBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                             nullptr, 0, wxGenericValidator(&m_alignment));
            alignChoice->Append(_(L"Left"));
            alignChoice->Append(_(L"Right"));
            alignChoice->Append(_(L"Center"));
            alignChoice->Append(_(L"Justified"));
            alignChoice->Append(_(L"Justified (at word)"));
            fontGrid->Add(alignChoice, wxSizerFlags{}.Expand());
            }

        fontGrid->Add(new wxStaticText(fontBox->GetStaticBox(), wxID_ANY, _(L"Line spacing:")),
                      wxSizerFlags{}.CenterVertical());
        m_lineSpacingSpin =
            new wxSpinCtrlDouble(fontBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                                 wxDefaultSize, wxSP_ARROW_KEYS, -100.0, 100.0, 1.0, 0.5);
        m_lineSpacingSpin->SetDigits(1);
        fontGrid->Add(m_lineSpacingSpin, wxSizerFlags{}.Expand());

        fontBox->Add(fontGrid, wxSizerFlags{}.Expand().Border());
        col1Sizer->Add(fontBox, wxSizerFlags{}.Expand().Border());

        // appearance options
        auto* appearanceBox = new wxStaticBoxSizer(wxVERTICAL, labelPage, _(L"Appearance"));
        auto* appearanceGrid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(4));
        appearanceGrid->AddGrowableCol(1, 1);

        appearanceGrid->Add(
            new wxStaticText(appearanceBox->GetStaticBox(), wxID_ANY, _(L"Orientation:")),
            wxSizerFlags{}.CenterVertical());
            {
            auto* orientationChoice =
                new wxChoice(appearanceBox->GetStaticBox(), wxID_ANY, wxDefaultPosition,
                             wxDefaultSize, 0, nullptr, 0, wxGenericValidator(&m_orientation));
            orientationChoice->Append(_(L"Horizontal"));
            orientationChoice->Append(_(L"Vertical"));
            appearanceGrid->Add(orientationChoice, wxSizerFlags{}.Expand());
            }

        appearanceGrid->Add(
            new wxStaticText(appearanceBox->GetStaticBox(), wxID_ANY, _(L"Visual style:")),
            wxSizerFlags{}.CenterVertical());
            {
            auto* styleChoice =
                new wxChoice(appearanceBox->GetStaticBox(), wxID_ANY, wxDefaultPosition,
                             wxDefaultSize, 0, nullptr, 0, wxGenericValidator(&m_labelStyle));
            // order must match Wisteria::LabelStyle enum
            styleChoice->Append(_(L"None"));
            styleChoice->Append(_(L"Index card"));
            styleChoice->Append(_(L"Lined paper"));
            styleChoice->Append(_(L"Lined paper (with margins)"));
            styleChoice->Append(_(L"Dotted lined paper"));
            styleChoice->Append(_(L"Dotted lined paper (with margins)"));
            styleChoice->Append(_(L"Right-arrow lined paper"));
            styleChoice->Append(_(L"Right-arrow lined paper (with margins)"));
            appearanceGrid->Add(styleChoice, wxSizerFlags{}.Expand());
            }

        appearanceBox->Add(appearanceGrid, wxSizerFlags{}.Expand().Border());
        col1Sizer->Add(appearanceBox, wxSizerFlags{}.Expand().Border());

        // header options
        auto* headerBox = new wxStaticBoxSizer(wxVERTICAL, labelPage, _(L"Header"));

        auto* enableHeaderCheck = new wxCheckBox(
            headerBox->GetStaticBox(), wxID_ANY, _(L"Treat first line as header"),
            wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&m_headerEnabled));
        headerBox->Add(enableHeaderCheck, wxSizerFlags{}.Border());

        auto* headerGrid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(4));
        headerGrid->AddGrowableCol(1, 1);

        headerGrid->Add(new wxStaticText(headerBox->GetStaticBox(), wxID_ANY, _(L"Font:")),
                        wxSizerFlags{}.CenterVertical());
        m_headerFontPicker = new wxFontPickerCtrl(
            headerBox->GetStaticBox(), wxID_ANY, wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
        headerGrid->Add(m_headerFontPicker, wxSizerFlags{}.Expand());

        headerGrid->Add(new wxStaticText(headerBox->GetStaticBox(), wxID_ANY, _(L"Color:")),
                        wxSizerFlags{}.CenterVertical());
        m_headerColorPicker = new wxColourPickerCtrl(headerBox->GetStaticBox(), wxID_ANY, *wxBLACK);
        headerGrid->Add(m_headerColorPicker, wxSizerFlags{}.Expand());

        headerGrid->Add(new wxStaticText(headerBox->GetStaticBox(), wxID_ANY, _(L"Alignment:")),
                        wxSizerFlags{}.CenterVertical());
        m_headerAlignmentChoice =
            new wxChoice(headerBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                         nullptr, 0, wxGenericValidator(&m_headerAlignment));
        m_headerAlignmentChoice->Append(_(L"Left"));
        m_headerAlignmentChoice->Append(_(L"Right"));
        m_headerAlignmentChoice->Append(_(L"Center"));
        m_headerAlignmentChoice->Append(_(L"Justified"));
        m_headerAlignmentChoice->Append(_(L"Justified (at word)"));
        headerGrid->Add(m_headerAlignmentChoice, wxSizerFlags{}.Expand());

        headerGrid->Add(
            new wxStaticText(headerBox->GetStaticBox(), wxID_ANY, _(L"Relative scaling:")),
            wxSizerFlags{}.CenterVertical());
        m_headerScalingSpin =
            new wxSpinCtrlDouble(headerBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                                 wxDefaultSize, wxSP_ARROW_KEYS, 0.5, 3.0, 1.0, 0.1);
        headerGrid->Add(m_headerScalingSpin, wxSizerFlags{}.Expand());

        headerBox->Add(headerGrid, wxSizerFlags{}.Expand().Border());
        col1Sizer->Add(headerBox, wxSizerFlags{}.Expand().Border());

        // column 2
        //---------

        // left image
        auto* leftImgBox = new wxStaticBoxSizer(wxVERTICAL, labelPage, _(L"Left Image"));
        auto* leftImgHBox = new wxBoxSizer(wxHORIZONTAL);
        m_leftImageThumbnail = new Thumbnail(
            leftImgBox->GetStaticBox(), wxNullBitmap, ClickMode::BrowseForImageFile, true, wxID_ANY,
            wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE | wxBORDER_SIMPLE);
        leftImgHBox->Add(m_leftImageThumbnail, wxSizerFlags{}.Border());
        auto* leftClearBtn = new wxButton(leftImgBox->GetStaticBox(), wxID_ANY, _(L"Clear"));
        leftImgHBox->Add(leftClearBtn, wxSizerFlags{}.Border());
        leftImgBox->Add(leftImgHBox, wxSizerFlags{}.Expand());

        // intercept click to capture the file path for round-tripping
        m_leftImageThumbnail->Bind(
            wxEVT_LEFT_DOWN,
            [this](wxMouseEvent&)
            {
                wxFileDialog fileDlg(
                    this, _(L"Select an Image"), wxString{}, wxString{},
                    wxString::Format(L"%s %s", _(L"Image Files"), wxImage::GetImageExtWildcard()),
                    wxFD_OPEN | wxFD_PREVIEW);
                if (fileDlg.ShowModal() == wxID_OK)
                    {
                    m_leftImagePath = fileDlg.GetPath();
                    m_leftImageThumbnail->LoadImage(m_leftImagePath);
                    }
            });
        leftClearBtn->Bind(wxEVT_BUTTON,
                           [this](wxCommandEvent&)
                           {
                               m_leftImagePath.clear();
                               m_leftImageThumbnail->SetBitmap(wxNullBitmap);
                           });
        col2Sizer->Add(leftImgBox, wxSizerFlags{}.Expand().Border());

        // top image
        auto* topImgBox = new wxStaticBoxSizer(wxVERTICAL, labelPage, _(L"Top Image"));
        auto* topImgHBox = new wxBoxSizer(wxHORIZONTAL);
        m_topImageThumbnail = new Thumbnail(
            topImgBox->GetStaticBox(), wxNullBitmap, ClickMode::BrowseForImageFile, true, wxID_ANY,
            wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE | wxBORDER_SIMPLE);
        topImgHBox->Add(m_topImageThumbnail, wxSizerFlags{}.Border());
        auto* topClearBtn = new wxButton(topImgBox->GetStaticBox(), wxID_ANY, _(L"Clear"));
        topImgHBox->Add(topClearBtn, wxSizerFlags{}.Border());
        topImgBox->Add(topImgHBox, wxSizerFlags{}.Expand());

        // intercept click to capture the file path for round-tripping
        m_topImageThumbnail->Bind(
            wxEVT_LEFT_DOWN,
            [this](wxMouseEvent&)
            {
                wxFileDialog fileDlg(
                    this, _(L"Select an Image"), wxString{}, wxString{},
                    wxString::Format(L"%s %s", _(L"Image Files"), wxImage::GetImageExtWildcard()),
                    wxFD_OPEN | wxFD_PREVIEW);
                if (fileDlg.ShowModal() == wxID_OK)
                    {
                    m_topImagePath = fileDlg.GetPath();
                    m_topImageThumbnail->LoadImage(m_topImagePath);
                    }
            });
        topClearBtn->Bind(wxEVT_BUTTON,
                          [this](wxCommandEvent&)
                          {
                              m_topImagePath.clear();
                              m_topImageThumbnail->SetBitmap(wxNullBitmap);
                          });
        col2Sizer->Add(topImgBox, wxSizerFlags{}.Expand().Border());

        // top shapes
        auto* topShapeBox = new wxStaticBoxSizer(wxVERTICAL, labelPage, _(L"Top Shapes"));

        m_topShapeListBox = new wxEditableListBox(
            topShapeBox->GetStaticBox(), wxID_ANY, _(L"Shapes:"), wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(100) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        topShapeBox->Add(m_topShapeListBox, wxSizerFlags{ 1 }.Expand().Border());

        // override New to open shape dialog
        m_topShapeListBox->GetNewButton()->Bind(wxEVT_BUTTON,
                                                [this](wxCommandEvent&) { OnAddTopShape(); });
        m_topShapeListBox->GetNewButton()->SetBitmapLabel(
            wxGetApp().ReadSvgIcon(L"shape.svg", wxSize{ 16, 16 }));

        // override Edit to open shape dialog with selected shape
        m_topShapeListBox->GetEditButton()->Bind(wxEVT_BUTTON,
                                                 [this](wxCommandEvent&) { OnEditTopShape(); });

        // override Delete to keep m_topShapes in sync
        m_topShapeListBox->GetDelButton()->Bind(
            wxEVT_BUTTON,
            [this](wxCommandEvent&)
            {
                const auto sel = m_topShapeListBox->GetListCtrl()->GetNextItem(
                    -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel != wxNOT_FOUND && std::cmp_less(sel, m_topShapes.size()))
                    {
                    m_topShapes.erase(std::next(m_topShapes.begin(), sel));
                    RefreshTopShapeList();
                    }
            });

        auto* offsetGrid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(4));
        offsetGrid->AddGrowableCol(1, 1);
        offsetGrid->Add(new wxStaticText(topShapeBox->GetStaticBox(), wxID_ANY,
                                         _(L"Top offset (image/shapes):")),
                        wxSizerFlags{}.CenterVertical());
        m_topShapeOffsetSpin = new wxSpinCtrl(topShapeBox->GetStaticBox(), wxID_ANY);
        m_topShapeOffsetSpin->SetRange(0, 1'000);
        m_topShapeOffsetSpin->SetValue(0);
        offsetGrid->Add(m_topShapeOffsetSpin, wxSizerFlags{}.Expand());

        topShapeBox->Add(offsetGrid, wxSizerFlags{}.Expand().Border());
        col2Sizer->Add(topShapeBox, wxSizerFlags{}.Expand().Border());

        // header controls start disabled
        OnEnableHeader(false);

        // bind events
        enableHeaderCheck->Bind(wxEVT_CHECKBOX,
                                [this](wxCommandEvent& evt) { OnEnableHeader(evt.IsChecked()); });
        enableBackgroundColorCheck->Bind(wxEVT_CHECKBOX,
                                         [this](wxCommandEvent& evt)
                                         {
                                             if (m_bgColorPicker != nullptr)
                                                 {
                                                 m_bgColorPicker->Enable(evt.IsChecked());
                                                 }
                                         });
        }

    //-------------------------------------------
    void InsertLabelDlg::OnEnableHeader(const bool enable)
        {
        if (m_headerFontPicker != nullptr)
            {
            m_headerFontPicker->Enable(enable);
            }
        if (m_headerColorPicker != nullptr)
            {
            m_headerColorPicker->Enable(enable);
            }
        if (m_headerAlignmentChoice != nullptr)
            {
            m_headerAlignmentChoice->Enable(enable);
            }
        if (m_headerScalingSpin != nullptr)
            {
            m_headerScalingSpin->Enable(enable);
            }
        }

    //-------------------------------------------
    void InsertLabelDlg::LoadFromLabel(const GraphItems::Label& label)
        {
        if (m_includePageOptions)
            {
            LoadPageOptions(label);
            }

        if (m_textCtrl != nullptr)
            {
            const auto textTemplate = label.GetPropertyTemplate(L"text");
            m_textCtrl->SetValue(textTemplate.empty() ? label.GetText() : textTemplate);
            }
        if (m_fontPicker != nullptr && label.GetFont().IsOk())
            {
            m_fontPicker->SetSelectedFont(label.GetFont());
            }
        if (m_fontColorPicker != nullptr && label.GetFontColor().IsOk())
            {
            m_fontColorPicker->SetColour(label.GetFontColor());
            }
        m_includeBackgroundColor = (label.GetFontBackgroundColor().IsOk() &&
                                    !label.GetFontBackgroundColor().IsTransparent());
        if (m_bgColorPicker != nullptr)
            {
            if (label.GetFontBackgroundColor().IsOk())
                {
                m_bgColorPicker->SetColour(label.GetFontBackgroundColor());
                }
            m_bgColorPicker->Enable(m_includeBackgroundColor);
            }
        m_alignment = static_cast<int>(label.GetTextAlignment());
        if (m_lineSpacingSpin != nullptr)
            {
            m_lineSpacingSpin->SetValue(label.GetLineSpacing());
            }

        // appearance
        m_orientation = (label.GetTextOrientation() == Wisteria::Orientation::Vertical) ? 1 : 0;
        m_labelStyle = static_cast<int>(label.GetLabelStyle());

        // header
        const auto& headerInfo = label.GetHeaderInfo();
        m_headerEnabled = headerInfo.IsEnabled();
        if (m_headerFontPicker != nullptr && headerInfo.GetFont().IsOk())
            {
            m_headerFontPicker->SetSelectedFont(headerInfo.GetFont());
            }
        if (m_headerColorPicker != nullptr && headerInfo.GetFontColor().IsOk())
            {
            m_headerColorPicker->SetColour(headerInfo.GetFontColor());
            }
        m_headerAlignment = static_cast<int>(headerInfo.GetLabelAlignment());
        if (m_headerScalingSpin != nullptr)
            {
            m_headerScalingSpin->SetValue(headerInfo.GetRelativeScaling());
            }

        OnEnableHeader(m_headerEnabled);

        // left image
        m_leftImagePath = label.GetPropertyTemplate(L"left-image.path");
        if (!m_leftImagePath.empty() && m_leftImageThumbnail != nullptr)
            {
            m_leftImageThumbnail->LoadImage(m_leftImagePath);
            }

        // top image
        m_topImagePath = label.GetPropertyTemplate(L"top-image.path");
        if (!m_topImagePath.empty() && m_topImageThumbnail != nullptr)
            {
            m_topImageThumbnail->LoadImage(m_topImagePath);
            }

        // top shapes
        const auto& topShapes = label.GetTopShape();
        if (topShapes.has_value())
            {
            m_topShapes = topShapes.value();
            }
        else
            {
            m_topShapes.clear();
            }
        if (m_topShapeOffsetSpin != nullptr)
            {
            m_topShapeOffsetSpin->SetValue(static_cast<int>(label.GetTopImageOffset()));
            }
        RefreshTopShapeList();

        TransferDataToWindow();
        }

    //-------------------------------------------
    void InsertLabelDlg::ApplyToLabel(Wisteria::GraphItems::Label& label)
        {
        label.SetText(GetLabelText());
        label.GetFont() = GetLabelFont();
        label.SetFontColor(GetFontColor());
        label.SetFontBackgroundColor(GetBackgroundColor());
        label.SetTextAlignment(GetLabelAlignment());
        label.SetLineSpacing(GetLineSpacing());
        label.SetTextOrientation(GetTextOrientation());
        label.SetLabelStyle(GetLabelStyle());

        auto& headerInfo = label.GetHeaderInfo();
        headerInfo.Enable(IsHeaderEnabled());
        if (IsHeaderEnabled())
            {
            headerInfo.Font(GetHeaderFont());
            headerInfo.FontColor(GetHeaderFontColor());
            headerInfo.LabelAlignment(GetHeaderAlignment());
            headerInfo.RelativeScaling(GetHeaderScaling());
            }

        // left image
        const auto leftImgPath = GetLeftImagePath();
        if (!leftImgPath.empty())
            {
            const auto img = Wisteria::GraphItems::Image::LoadFile(leftImgPath);
            if (img.IsOk())
                {
                label.SetLeftImage(wxBitmapBundle::FromImage(img));
                label.SetPropertyTemplate(L"left-image.path", leftImgPath);
                }
            }
        else
            {
            label.SetLeftImage(wxBitmapBundle{});
            label.SetPropertyTemplate(L"left-image.path", wxString{});
            }

        // top image
        const auto topImgPath = GetTopImagePath();
        if (!topImgPath.empty())
            {
            const auto img = Wisteria::GraphItems::Image::LoadFile(topImgPath);
            if (img.IsOk())
                {
                label.SetTopImage(wxBitmapBundle::FromImage(img),
                                  static_cast<size_t>(GetTopShapeOffset()));
                label.SetPropertyTemplate(L"top-image.path", topImgPath);
                }
            }
        else
            {
            label.SetTopImage(wxBitmapBundle{});
            label.SetPropertyTemplate(L"top-image.path", wxString{});
            }

        // top shapes (shares offset with top image)
        if (!m_topShapes.empty())
            {
            label.SetTopShape(m_topShapes, static_cast<size_t>(GetTopShapeOffset()));
            }
        else
            {
            label.SetTopShape(std::nullopt);
            }
        }

    //-------------------------------------------
    int InsertLabelDlg::GetTopShapeOffset() const
        {
        return m_topShapeOffsetSpin != nullptr ? m_topShapeOffsetSpin->GetValue() : 0;
        }

    //-------------------------------------------
    wxString InsertLabelDlg::FormatShapeLabel(const Wisteria::GraphItems::ShapeInfo& shp)
        {
        const auto nameOpt = Wisteria::ReportEnumConvert::ConvertIconToString(shp.GetShape());
        const auto name = nameOpt.has_value() ? nameOpt.value() : _(L"Unknown");
        const auto sz = shp.GetSizeDIPs();
        return wxString::Format(L"%s (%d×%d)", name, sz.GetWidth(), sz.GetHeight());
        }

    //-------------------------------------------
    void InsertLabelDlg::RefreshTopShapeList()
        {
        if (m_topShapeListBox == nullptr)
            {
            return;
            }

        wxArrayString items;
        items.reserve(m_topShapes.size());
        for (const auto& shp : m_topShapes)
            {
            items.Add(FormatShapeLabel(shp));
            }
        m_topShapeListBox->SetStrings(items);
        }

    //-------------------------------------------
    void InsertLabelDlg::OnAddTopShape()
        {
        InsertShapeDlg dlg(
            GetCanvas(), nullptr, this, _(L"Add Top Shape"), wxID_ANY, wxDefaultPosition,
            wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
            InsertItemDlg::EditMode::Insert, ShapeDlgIncludeMost & ~ShapeDlgIncludeAlignment);
        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        Wisteria::GraphItems::ShapeInfo shp;
        shp.Shape(dlg.GetIconShape())
            .Size(wxSize{ dlg.GetShapeWidth(), dlg.GetShapeHeight() })
            .Pen(wxPen{ dlg.GetPenColor(), dlg.GetPenWidth(), dlg.GetPenStyle() })
            .Brush(wxBrush{ dlg.GetBrushColor(), dlg.GetBrushStyle() })
            .Text(dlg.GetLabelText());
        if (dlg.IsFillable())
            {
            shp.FillPercent(dlg.GetFillPercent());
            }

        m_topShapes.push_back(shp);
        RefreshTopShapeList();
        }

    //-------------------------------------------
    void InsertLabelDlg::OnEditTopShape()
        {
        if (m_topShapeListBox == nullptr)
            {
            return;
            }
        const auto sel = m_topShapeListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                       wxLIST_STATE_SELECTED);
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_topShapes.size()))
            {
            return;
            }

        InsertShapeDlg dlg(
            GetCanvas(), nullptr, this, _(L"Edit Top Shape"), wxID_ANY, wxDefaultPosition,
            wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
            InsertItemDlg::EditMode::Edit, ShapeDlgIncludeMost & ~ShapeDlgIncludeAlignment);
        dlg.LoadFromShapeInfo(m_topShapes[static_cast<size_t>(sel)]);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        Wisteria::GraphItems::ShapeInfo shp;
        shp.Shape(dlg.GetIconShape())
            .Size(wxSize{ dlg.GetShapeWidth(), dlg.GetShapeHeight() })
            .Pen(wxPen{ dlg.GetPenColor(), dlg.GetPenWidth(), dlg.GetPenStyle() })
            .Brush(wxBrush{ dlg.GetBrushColor(), dlg.GetBrushStyle() })
            .Text(dlg.GetLabelText());
        if (dlg.IsFillable())
            {
            shp.FillPercent(dlg.GetFillPercent());
            }

        m_topShapes[static_cast<size_t>(sel)] = shp;
        RefreshTopShapeList();
        }

    } // namespace Wisteria::UI
