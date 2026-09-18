///////////////////////////////////////////////////////////////////////////////
// Name:        insertkpicarddlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertkpicarddlg.h"
#include "../../base/label.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    /// @brief Orientation/style settings for a KPI card layout preset.
    struct KpiPresetSettings
        {
        Wisteria::Orientation m_orientation{ Wisteria::Orientation::Horizontal };
        Wisteria::LabelStyle m_style{ Wisteria::LabelStyle::NoLabelStyle };
        Wisteria::LabelShape m_shape{ Wisteria::LabelShape::NoShape };
        Wisteria::LabelBoundingBoxContentAdjustment m_boxAdjust{
            Wisteria::LabelBoundingBoxContentAdjustment::ContentAdjustNone
        };
        double m_headerScaling{ 3.0 };
        // where the content sits within its cell
        Wisteria::PageHorizontalAlignment m_pageHAlign{
            Wisteria::PageHorizontalAlignment::Centered
        };
        Wisteria::PageVerticalAlignment m_pageVAlign{ Wisteria::PageVerticalAlignment::Centered };
        };

    //-------------------------------------------
    static KpiPresetSettings GetKpiPresetSettings(const int presetIndex)
        {
        switch (presetIndex)
            {
        case 1: // Index Card
            // shrinks the width to the content
            return { Wisteria::Orientation::Horizontal,
                     Wisteria::LabelStyle::IndexCard,
                     Wisteria::LabelShape::NoShape,
                     Wisteria::LabelBoundingBoxContentAdjustment::ContentAdjustWidth,
                     3.0,
                     Wisteria::PageHorizontalAlignment::Centered,
                     Wisteria::PageVerticalAlignment::Centered };
        default: // Standard
            return { Wisteria::Orientation::Horizontal,
                     Wisteria::LabelStyle::NoLabelStyle,
                     Wisteria::LabelShape::NoShape,
                     Wisteria::LabelBoundingBoxContentAdjustment::ContentAdjustNone,
                     3.0,
                     Wisteria::PageHorizontalAlignment::Centered,
                     Wisteria::PageVerticalAlignment::Centered };
            }
        }

    //-------------------------------------------
    InsertKpiCardDlg::InsertKpiCardDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                       wxWindow* parent, const wxString& caption,
                                       const wxWindowID id, const wxPoint& pos, const wxSize& size,
                                       const long style)
        : InsertLabelDlg(DeferredConstructionTag{}, canvas, reportBuilder, parent, caption, id, pos,
                         size, style, EditMode::Insert, LabelDlgIncludeShapeAndPageOptions)
        {
        CreateControls();
        FinalizeControls();
        TransferDataToWindow();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertKpiCardDlg::CreateControls()
        {
        InsertItemDlg::CreateControls();

        CreateKpiPage();
        CreateShapesPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertKpiCardDlg::CreateKpiPage()
        {
        auto* kpiPage = new wxPanel(GetSideBarBook());
        auto* kpiSizer = new wxBoxSizer(wxVERTICAL);
        kpiPage->SetSizer(kpiSizer);
        GetSideBarBook()->AddPage(kpiPage, _(L"KPI Card"), ID_KPI_SECTION, true);

        auto* contentBox = new wxStaticBoxSizer(wxVERTICAL, kpiPage, _(L"Content"));
        auto* contentGrid = new wxFlexGridSizer(2, wxSizerFlags::GetDefaultBorder() * 2,
                                                wxSizerFlags::GetDefaultBorder());
        contentGrid->AddGrowableCol(1, 1);

        contentGrid->Add(new wxStaticText(contentBox->GetStaticBox(), wxID_ANY, _(L"Big number:")),
                         wxSizerFlags{}.CenterVertical());
        m_bigNumberCtrl = new wxTextCtrl(contentBox->GetStaticBox(), wxID_ANY, wxString{},
                                         wxDefaultPosition, wxDefaultSize, wxTE_RICH2);
#if wxUSE_SPELLCHECK
        m_bigNumberCtrl->EnableProofCheck(wxTextProofOptions::Default().GrammarCheck());
#endif
        contentGrid->Add(m_bigNumberCtrl, wxSizerFlags{}.Expand());

        contentGrid->Add(
            new wxStaticText(contentBox->GetStaticBox(), wxID_ANY, _(L"Number color:")),
            wxSizerFlags{}.CenterVertical());
        m_numberColorPicker =
            new wxColourPickerCtrl(contentBox->GetStaticBox(), wxID_ANY, *wxBLACK);
        contentGrid->Add(m_numberColorPicker, wxSizerFlags{}.Expand());

        contentGrid->Add(new wxStaticText(contentBox->GetStaticBox(), wxID_ANY, _(L"Caption:")),
                         wxSizerFlags{}.Top());
        m_captionCtrl =
            new wxTextCtrl(contentBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxSize{ FromDIP(300), FromDIP(80) }, wxTE_MULTILINE | wxTE_RICH2);
#if wxUSE_SPELLCHECK
        m_captionCtrl->EnableProofCheck(wxTextProofOptions::Default().GrammarCheck());
#endif
        contentGrid->Add(m_captionCtrl, wxSizerFlags{}.Expand());

        contentGrid->Add(
            new wxStaticText(contentBox->GetStaticBox(), wxID_ANY, _(L"Caption color:")),
            wxSizerFlags{}.CenterVertical());
        m_captionColorPicker =
            new wxColourPickerCtrl(contentBox->GetStaticBox(), wxID_ANY, *wxBLACK);
        contentGrid->Add(m_captionColorPicker, wxSizerFlags{}.Expand());

        contentBox->Add(contentGrid, wxSizerFlags{}.Expand().Border());
        kpiSizer->Add(contentBox, wxSizerFlags{}.Expand().Border());

        auto* layoutBox = new wxStaticBoxSizer(wxVERTICAL, kpiPage, _(L"Layout"));
        auto* layoutGrid = new wxFlexGridSizer(2, wxSizerFlags::GetDefaultBorder() * 2,
                                               wxSizerFlags::GetDefaultBorder());
        layoutGrid->AddGrowableCol(1, 1);

        layoutGrid->Add(new wxStaticText(layoutBox->GetStaticBox(), wxID_ANY, _(L"Preset:")),
                        wxSizerFlags{}.CenterVertical());
        auto* presetChoice =
            new wxChoice(layoutBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                         nullptr, 0, wxGenericValidator{ &m_layoutPreset });
        // order must match GetKpiPresetSettings()
        presetChoice->Append(_(L"Standard"));
        presetChoice->Append(_(L"Index Card"));
        layoutGrid->Add(presetChoice, wxSizerFlags{}.Expand());

        // order must match TextAlignment
        const wxArrayString alignmentNames{ _(L"Left"), _(L"Right"), _(L"Center"), _(L"Justified"),
                                            _(L"Justified (at word)") };

        layoutGrid->Add(
            new wxStaticText(layoutBox->GetStaticBox(), wxID_ANY, _(L"Number alignment:")),
            wxSizerFlags{}.CenterVertical());
        layoutGrid->Add(new wxChoice(layoutBox->GetStaticBox(), wxID_ANY, wxDefaultPosition,
                                     wxDefaultSize, alignmentNames, 0,
                                     wxGenericValidator{ &m_numberAlignment }),
                        wxSizerFlags{}.Expand());

        layoutGrid->Add(
            new wxStaticText(layoutBox->GetStaticBox(), wxID_ANY, _(L"Caption alignment:")),
            wxSizerFlags{}.CenterVertical());
        layoutGrid->Add(new wxChoice(layoutBox->GetStaticBox(), wxID_ANY, wxDefaultPosition,
                                     wxDefaultSize, alignmentNames, 0,
                                     wxGenericValidator{ &m_captionAlignment }),
                        wxSizerFlags{}.Expand());

        layoutBox->Add(layoutGrid, wxSizerFlags{}.Expand().Border());
        kpiSizer->Add(layoutBox, wxSizerFlags{}.Expand().Border());
        }

    //-------------------------------------------
    std::shared_ptr<Wisteria::GraphItems::Label> InsertKpiCardDlg::BuildKpiCard()
        {
        const auto bigNumber =
            m_bigNumberCtrl != nullptr ? m_bigNumberCtrl->GetValue() : wxString{};
        const auto captionText = m_captionCtrl != nullptr ? m_captionCtrl->GetValue() : wxString{};

        wxString rawText{ bigNumber };
        if (!captionText.empty())
            {
            rawText += L"\n" + captionText;
            }

        auto label =
            std::make_shared<Wisteria::GraphItems::Label>(Wisteria::GraphItems::GraphItemInfo{});

        ApplyPageOptions(*label);
        ApplyAccessibilityOptions(*label);
        ApplyShapeOptionsToLabel(*label);

        const auto preset = GetKpiPresetSettings(m_layoutPreset);
        // the preset's alignment overrides the Placement page's
        label->SetPageHorizontalAlignment(preset.m_pageHAlign);
        label->SetPageVerticalAlignment(preset.m_pageVAlign);

        auto defaultFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
        GraphItems::Label::FixFont(defaultFont);
        label->GetFont() = defaultFont;
        label->SetFontColor(m_captionColorPicker != nullptr ? m_captionColorPicker->GetColour() :
                                                              *wxBLACK);
        label->SetTextAlignment(static_cast<Wisteria::TextAlignment>(m_captionAlignment));
        label->SetTextOrientation(preset.m_orientation);
        label->SetLabelStyle(preset.m_style);
        label->SetShape(preset.m_shape);
        label->SetBoundingBoxToContentAdjustment(preset.m_boxAdjust);

        auto headerFont = defaultFont;
        headerFont.MakeBold();
        auto& headerInfo = label->GetHeaderInfo();
        headerInfo.Enable(true);
        headerInfo.Font(headerFont);
        GraphItems::Label::FixFont(headerInfo.GetFont());
        headerInfo.FontColor(m_numberColorPicker != nullptr ? m_numberColorPicker->GetColour() :
                                                              *wxBLACK);
        headerInfo.LabelAlignment(static_cast<Wisteria::TextAlignment>(m_numberAlignment));
        headerInfo.RelativeScaling(preset.m_headerScaling);

        if (GetReportBuilder() != nullptr)
            {
            const auto expanded = GetReportBuilder()->ExpandConstants(rawText);
            label->SetPropertyTemplate(L"text", (expanded != rawText) ? rawText : wxString{});
            label->SetText(expanded);
            }
        else
            {
            label->SetPropertyTemplate(L"text", wxString{});
            label->SetText(rawText);
            }

        label->SetDPIScaleFactor(GetCanvas()->FromDIP(1));

        return label;
        }
    } // namespace Wisteria::UI
