///////////////////////////////////////////////////////////////////////////////
// Name:        insertcatbarchartdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertcatbarchartdlg.h"
#include "../../base/reportenumconvert.h"
#include "insertimgdlg.h"
#include "insertshapedlg.h"
#include "variableselectdlg.h"
#include <wx/filename.h>
#include <wx/tokenzr.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertCatBarChartDlg::InsertCatBarChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                               wxWindow* parent, const wxString& caption,
                                               const wxWindowID id, const wxPoint& pos,
                                               const wxSize& size, const long style,
                                               EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        const auto currentSize = GetSize();
        SetSize(currentSize.GetWidth() * 2, currentSize.GetHeight());
        SetMinSize(wxSize{ currentSize.GetWidth() * 2, currentSize.GetHeight() });

        Centre();
        }

    //-------------------------------------------
    void InsertCatBarChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Bar Chart Options"), ID_OPTIONS_SECTION, true);

        // dataset selector
        auto* datasetSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        datasetSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Dataset:")),
                          wxSizerFlags{}.CenterVertical());
        m_datasetChoice = new wxChoice(optionsPage, ID_DATASET_CHOICE);
        datasetSizer->Add(m_datasetChoice);

        // populate dataset names from the report builder
        if (GetReportBuilder() != nullptr)
            {
            for (const auto& [name, dataset] : GetReportBuilder()->GetDatasets())
                {
                m_datasetNames.push_back(name);
                m_datasetChoice->Append(name);
                }
            }
        if (!m_datasetNames.empty())
            {
            m_datasetChoice->SetSelection(0);
            }

        optionsSizer->Add(datasetSizer, wxSizerFlags{}.Border());

        // variables button
        auto* varsBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Variables"));
        auto* varButton =
            new wxButton(varsBox->GetStaticBox(), ID_SELECT_VARS_BUTTON, _(L"Select..."));
        varsBox->Add(varButton, wxSizerFlags{}.Border(wxLEFT));

        // variable label grid
        auto* varGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(12), FromDIP(2) });

        auto* catLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Category:"));
        catLabel->SetFont(catLabel->GetFont().Bold());
        varGrid->Add(catLabel, wxSizerFlags{}.CenterVertical());
        m_categoricalVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_categoricalVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_categoricalVarLabel, wxSizerFlags{}.CenterVertical());

        auto* weightLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Weight (optional):"));
        weightLabel->SetFont(weightLabel->GetFont().Bold());
        varGrid->Add(weightLabel, wxSizerFlags{}.CenterVertical());
        m_weightVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_weightVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_weightVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Group (optional):"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // color scheme
        auto* colorSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        colorSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Color scheme:")),
                        wxSizerFlags{}.CenterVertical());
        colorSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     GetColorSchemeNames(), 0,
                                     wxGenericValidator(&m_colorSchemeIndex)),
                        wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(colorSizer, wxSizerFlags{}.Border());

        // bar orientation
        auto* orientSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        orientSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Orientation:")),
                         wxSizerFlags{}.CenterVertical());
        wxArrayString orientations;
        orientations.Add(_(L"Horizontal"));
        orientations.Add(_(L"Vertical"));
        orientSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      orientations, 0, wxGenericValidator(&m_barOrientationIndex)),
                         wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(orientSizer, wxSizerFlags{}.Border());

        // bar label display
        auto* labelDispSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        labelDispSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Bar labels:")),
                            wxSizerFlags{}.CenterVertical());
        wxArrayString labelDisplays;
        labelDisplays.Add(_(L"Value"));
        labelDisplays.Add(_(L"Percentage"));
        labelDisplays.Add(_(L"Value and Percentage"));
        labelDisplays.Add(_(L"No Display"));
        labelDisplays.Add(_(L"Name"));
        labelDisplays.Add(_(L"Name and Value"));
        labelDisplays.Add(_(L"Name and Percentage"));
        labelDispSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         labelDisplays, 0,
                                         wxGenericValidator(&m_barLabelDisplayIndex)),
                            wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(labelDispSizer, wxSizerFlags{}.Border());

        // box effect
        auto* effectSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        effectSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Box effect:")),
                         wxSizerFlags{}.CenterVertical());
        wxArrayString boxEffects;
        boxEffects.Add(_(L"Solid"));
        boxEffects.Add(_(L"Glassy"));
        boxEffects.Add(_(L"Fade from Bottom to Top"));
        boxEffects.Add(_(L"Fade from Top to Bottom"));
        boxEffects.Add(_(L"Stipple Image"));
        boxEffects.Add(_(L"Stipple Shape"));
        boxEffects.Add(_(L"Watercolor"));
        boxEffects.Add(_(L"Thick Watercolor"));
        boxEffects.Add(_(L"Common Image"));
        boxEffects.Add(_(L"Image"));
        boxEffects.Add(_(L"Marker"));
        boxEffects.Add(_(L"Pencil"));
        m_boxEffectChoice = new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                         boxEffects, 0, wxGenericValidator(&m_boxEffectIndex));
        effectSizer->Add(m_boxEffectChoice, wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(effectSizer, wxSizerFlags{}.Border());

        // stipple shape button and label (enabled only for StippleShape effect)
        auto* shapeBtnSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        m_shapeButton = new wxButton(optionsPage, wxID_ANY, _(L"Shape..."));
        m_shapeButton->Enable(false);
        shapeBtnSizer->Add(m_shapeButton, wxSizerFlags{}.CenterVertical());
        m_shapeLabel = new wxStaticText(optionsPage, wxID_ANY, wxString{});
        m_shapeLabel->SetForegroundColour(GetVariableLabelColor());
        m_shapeLabel->Enable(false);
        shapeBtnSizer->Add(m_shapeLabel, wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(shapeBtnSizer, wxSizerFlags{}.Border(wxLEFT));

        // images button and label (enabled only for image-based effects)
        auto* imgBtnSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        m_imagesButton = new wxButton(optionsPage, wxID_ANY, _(L"Images..."));
        m_imagesButton->Enable(false);
        imgBtnSizer->Add(m_imagesButton, wxSizerFlags{}.CenterVertical());
        m_imagesLabel = new wxStaticText(optionsPage, wxID_ANY, wxString{});
        m_imagesLabel->SetForegroundColour(GetVariableLabelColor());
        m_imagesLabel->Enable(false);
        imgBtnSizer->Add(m_imagesLabel, wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(imgBtnSizer, wxSizerFlags{}.Border(wxLEFT));

        // legend placement
        auto* legendGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        m_legendLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"Legend:"));
        m_legendLabel->Enable(false);
        legendGrid->Add(m_legendLabel, wxSizerFlags{}.CenterVertical());
        m_legendChoice = CreateLegendPlacementChoice(optionsPage, 1);
        m_legendChoice->Enable(false);
        legendGrid->Add(m_legendChoice);
        optionsSizer->Add(legendGrid, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        m_boxEffectChoice->Bind(wxEVT_CHOICE,
                                [this]([[maybe_unused]] wxCommandEvent&) { OnBoxEffectChanged(); });

        m_shapeButton->Bind(wxEVT_BUTTON,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnSelectStippleShape(); });

        m_imagesButton->Bind(wxEVT_BUTTON,
                             [this]([[maybe_unused]] wxCommandEvent&) { OnSelectImages(); });
        }

    //-------------------------------------------
    BoxEffect InsertCatBarChartDlg::BoxEffectFromIndex(const int index) noexcept
        {
        switch (index)
            {
        case 1:
            return BoxEffect::Glassy;
        case 2:
            return BoxEffect::FadeFromBottomToTop;
        case 3:
            return BoxEffect::FadeFromTopToBottom;
        case 4:
            return BoxEffect::StippleImage;
        case 5:
            return BoxEffect::StippleShape;
        case 6:
            return BoxEffect::WaterColor;
        case 7:
            return BoxEffect::ThickWaterColor;
        case 8:
            return BoxEffect::CommonImage;
        case 9:
            return BoxEffect::Image;
        case 10:
            return BoxEffect::Marker;
        case 11:
            return BoxEffect::Pencil;
        case 0:
            [[fallthrough]];
        default:
            return BoxEffect::Solid;
            }
        }

    //-------------------------------------------
    int InsertCatBarChartDlg::BoxEffectToIndex(const BoxEffect effect) noexcept
        {
        switch (effect)
            {
        case BoxEffect::Glassy:
            return 1;
        case BoxEffect::FadeFromBottomToTop:
            return 2;
        case BoxEffect::FadeFromTopToBottom:
            return 3;
        case BoxEffect::StippleImage:
            return 4;
        case BoxEffect::StippleShape:
            return 5;
        case BoxEffect::WaterColor:
            return 6;
        case BoxEffect::ThickWaterColor:
            return 7;
        case BoxEffect::CommonImage:
            return 8;
        case BoxEffect::Image:
            return 9;
        case BoxEffect::Marker:
            return 10;
        case BoxEffect::Pencil:
            return 11;
        case BoxEffect::Solid:
            [[fallthrough]];
        default:
            return 0;
            }
        }

    //-------------------------------------------
    BinLabelDisplay InsertCatBarChartDlg::BinLabelDisplayFromIndex(const int index) noexcept
        {
        switch (index)
            {
        case 1:
            return BinLabelDisplay::BinPercentage;
        case 2:
            return BinLabelDisplay::BinValueAndPercentage;
        case 3:
            return BinLabelDisplay::NoDisplay;
        case 4:
            return BinLabelDisplay::BinName;
        case 5:
            return BinLabelDisplay::BinNameAndValue;
        case 6:
            return BinLabelDisplay::BinNameAndPercentage;
        case 0:
            [[fallthrough]];
        default:
            return BinLabelDisplay::BinValue;
            }
        }

    //-------------------------------------------
    int InsertCatBarChartDlg::BinLabelDisplayToIndex(const BinLabelDisplay display) noexcept
        {
        switch (display)
            {
        case BinLabelDisplay::BinPercentage:
            return 1;
        case BinLabelDisplay::BinValueAndPercentage:
            return 2;
        case BinLabelDisplay::NoDisplay:
            return 3;
        case BinLabelDisplay::BinName:
            return 4;
        case BinLabelDisplay::BinNameAndValue:
            return 5;
        case BinLabelDisplay::BinNameAndPercentage:
            return 6;
        case BinLabelDisplay::BinValue:
            [[fallthrough]];
        default:
            return 0;
            }
        }

    //-------------------------------------------
    void InsertCatBarChartDlg::OnBoxEffectChanged()
        {
        const int sel = m_boxEffectChoice->GetSelection();
        // StippleShape is index 5
        m_shapeButton->Enable(sel == 5);
        m_shapeLabel->Enable(sel == 5);
        // StippleImage (4), CommonImage (8), Image (9)
        m_imagesButton->Enable(sel == 4 || sel == 8 || sel == 9);
        m_imagesLabel->Enable(sel == 4 || sel == 8 || sel == 9);
        }

    //-------------------------------------------
    void InsertCatBarChartDlg::OnSelectStippleShape()
        {
        InsertShapeDlg dlg(GetCanvas(), GetReportBuilder(), this, _(L"Select Stipple Shape"),
                           wxID_ANY, wxDefaultPosition, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                           InsertItemDlg::EditMode::Edit,
                           ShapeDlgIncludeBrush | ShapeDlgIncludeLabel);

        // pre-populate with previously selected settings
        dlg.SetIconShape(m_stippleShape);
        dlg.SetBrushColor(m_stippleShapeColor);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        m_stippleShape = dlg.GetIconShape();
        m_stippleShapeColor = dlg.GetBrushColor();

        const auto shapeStr = Wisteria::ReportEnumConvert::ConvertIconToString(m_stippleShape);
        m_shapeLabel->SetLabel(shapeStr.value_or(wxString{}));
        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    void InsertCatBarChartDlg::OnSelectImages()
        {
        InsertImageDlg dlg(GetCanvas(), GetReportBuilder(), this, _(L"Select Images"), wxID_ANY,
                           wxDefaultPosition, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                           InsertItemDlg::EditMode::Edit, ImageDlgIncludeEffect);

        // pre-populate with previously selected settings
        if (!m_imagePaths.empty())
            {
            dlg.SetImagePaths(m_imagePaths);
            }
        dlg.SetCustomSize(m_imageCustomSize, m_imageWidth, m_imageHeight);
        dlg.SetResizeMethod(m_imageResizeMethod);
        dlg.SetImageEffect(m_imageEffect);
        dlg.SetStitchDirection(m_imageStitchDirection);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        m_imagePaths = dlg.GetImagePaths();
        m_imageCustomSize = dlg.IsCustomSizeEnabled();
        m_imageWidth = dlg.GetImageWidth();
        m_imageHeight = dlg.GetImageHeight();
        m_imageResizeMethod = dlg.GetResizeMethod();
        m_imageEffect = dlg.GetImageEffect();
        m_imageStitchDirection = dlg.GetStitchDirection();

        if (m_imagePaths.empty())
            {
            m_imagesLabel->SetLabel(wxString{});
            }
        else if (m_imagePaths.GetCount() == 1)
            {
            m_imagesLabel->SetLabel(wxFileName(m_imagePaths[0]).GetFullName());
            }
        else
            {
            m_imagesLabel->SetLabel(
                wxString::Format(wxPLURAL(L"%zu image", L"%zu images", m_imagePaths.GetCount()),
                                 m_imagePaths.GetCount()));
            }
        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    void InsertCatBarChartDlg::OnDatasetChanged()
        {
        m_categoricalVariable.clear();
        m_weightVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertCatBarChartDlg::OnSelectVariables()
        {
        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset first."), _(L"No Dataset"),
                         wxOK | wxICON_INFORMATION, this);
            return;
            }

        // prefer the stored column preview info (preserves original file order)
        Data::Dataset::ColumnPreviewInfo columnInfo;
        if (GetReportBuilder() != nullptr)
            {
            const auto& importOpts = GetReportBuilder()->GetDatasetImportOptions();
            const int sel = m_datasetChoice->GetSelection();
            if (sel != wxNOT_FOUND && std::cmp_less(sel, m_datasetNames.size()))
                {
                const auto it = importOpts.find(m_datasetNames[sel]);
                if (it != importOpts.cend())
                    {
                    columnInfo = it->second.m_columnPreviewInfo;
                    }
                }
            }
        if (columnInfo.empty())
            {
            columnInfo = BuildColumnPreviewInfo(*dataset);
            }

        using VLI = VariableSelectDlg::VariableListInfo;
        VariableSelectDlg dlg(
            this, columnInfo,
            { VLI{}
                  .Label(_(L"Category"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_categoricalVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_categoricalVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Weight"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_weightVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_weightVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Group"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_groupVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_groupVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto catVars = dlg.GetSelectedVariables(0);
        m_categoricalVariable = catVars.empty() ? wxString{} : catVars.front();

        const auto weightVars = dlg.GetSelectedVariables(1);
        m_weightVariable = weightVars.empty() ? wxString{} : weightVars.front();

        const auto groupVars = dlg.GetSelectedVariables(2);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertCatBarChartDlg::UpdateVariableLabels()
        {
        m_categoricalVarLabel->SetLabel(m_categoricalVariable);
        m_weightVarLabel->SetLabel(m_weightVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        const bool hasGroup = !m_groupVariable.empty();
        if (m_legendLabel != nullptr)
            {
            m_legendLabel->Enable(hasGroup);
            }
        if (m_legendChoice != nullptr)
            {
            m_legendChoice->Enable(hasGroup);
            if (!hasGroup)
                {
                m_legendChoice->SetSelection(0);
                }
            }

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertCatBarChartDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
        {
        Data::Dataset::ColumnPreviewInfo info;

        for (const auto& col : dataset.GetContinuousColumns())
            {
            info.emplace_back(col.GetName(), Data::Dataset::ColumnImportType::Numeric, wxString{});
            }
        for (const auto& col : dataset.GetCategoricalColumns())
            {
            info.emplace_back(col.GetName(), Data::Dataset::ColumnImportType::String, wxString{});
            }
        for (const auto& col : dataset.GetDateColumns())
            {
            info.emplace_back(col.GetName(), Data::Dataset::ColumnImportType::Date, wxString{});
            }

        return info;
        }

    //-------------------------------------------
    std::shared_ptr<Data::Dataset> InsertCatBarChartDlg::GetSelectedDataset() const
        {
        if (GetReportBuilder() == nullptr || m_datasetChoice == nullptr)
            {
            return nullptr;
            }

        const int sel = m_datasetChoice->GetSelection();
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_datasetNames.size()))
            {
            return nullptr;
            }

        const auto& datasets = GetReportBuilder()->GetDatasets();
        const auto it = datasets.find(m_datasetNames[sel]);
        return (it != datasets.cend()) ? it->second : nullptr;
        }

    //-------------------------------------------
    bool InsertCatBarChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_categoricalVariable.empty())
            {
            wxMessageBox(_(L"Please select a categorical variable."), _(L"Variable Not Specified"),
                         wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertCatBarChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* barChart = dynamic_cast<const Graphs::CategoricalBarChart*>(&graph);
        if (barChart == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = barChart->GetPropertyTemplate(L"dataset");
        if (!dsName.empty() && m_datasetChoice != nullptr)
            {
            for (size_t i = 0; i < m_datasetNames.size(); ++i)
                {
                if (m_datasetNames[i] == dsName)
                    {
                    m_datasetChoice->SetSelection(static_cast<int>(i));
                    break;
                    }
                }
            }

        // load column names from property templates (fall back to getters)
        m_categoricalVariable = barChart->GetPropertyTemplate(L"variables.category");
        if (m_categoricalVariable.empty())
            {
            m_categoricalVariable = barChart->GetCategoricalColumnName();
            }

        m_weightVariable = barChart->GetPropertyTemplate(L"variables.aggregate");
        if (m_weightVariable.empty())
            {
            m_weightVariable = barChart->GetWeightColumnName().value_or(wxString{});
            }

        m_groupVariable = barChart->GetPropertyTemplate(L"variables.group");
        if (m_groupVariable.empty())
            {
            m_groupVariable = barChart->GetGroupColumnName().value_or(wxString{});
            }

        UpdateVariableLabels();

        // bar chart options
        m_boxEffectIndex = BoxEffectToIndex(barChart->GetBarEffect());
        m_barOrientationIndex = (barChart->GetBarOrientation() == Orientation::Vertical) ? 1 : 0;
        m_barLabelDisplayIndex = BinLabelDisplayToIndex(barChart->GetBinLabelDisplay());

        // stipple shape (restore from property templates so it persists
        // even when the current box effect is not StippleShape)
        const auto savedShape = barChart->GetPropertyTemplate(L"stipple-shape");
        if (!savedShape.empty())
            {
            const auto shape = Wisteria::ReportEnumConvert::ConvertIcon(savedShape);
            if (shape)
                {
                m_stippleShape = shape.value();
                }
            }
        const auto savedColor = barChart->GetPropertyTemplate(L"stipple-shape-color");
        if (!savedColor.empty())
            {
            const wxColour clr{ savedColor };
            if (clr.IsOk())
                {
                m_stippleShapeColor = clr;
                }
            }
        const auto shapeStr = Wisteria::ReportEnumConvert::ConvertIconToString(m_stippleShape);
        m_shapeLabel->SetLabel(shapeStr.value_or(wxString{}));

        // image settings (restore from property templates)
        const auto savedPaths = barChart->GetPropertyTemplate(L"image-paths");
        if (!savedPaths.empty())
            {
            wxStringTokenizer tokenizer(savedPaths, L"\t");
            while (tokenizer.HasMoreTokens())
                {
                m_imagePaths.Add(tokenizer.GetNextToken());
                }
            if (m_imagePaths.GetCount() == 1)
                {
                m_imagesLabel->SetLabel(wxFileName(m_imagePaths[0]).GetFullName());
                }
            else if (m_imagePaths.GetCount() > 1)
                {
                m_imagesLabel->SetLabel(
                    wxString::Format(wxPLURAL(L"%zu image", L"%zu images", m_imagePaths.GetCount()),
                                     m_imagePaths.GetCount()));
                }
            }

        // image custom size
        const auto savedImgWidth = barChart->GetPropertyTemplate(L"image-width");
        const auto savedImgHeight = barChart->GetPropertyTemplate(L"image-height");
        if (!savedImgWidth.empty() || !savedImgHeight.empty())
            {
            m_imageCustomSize = true;
            if (!savedImgWidth.empty())
                {
                m_imageWidth = wxAtoi(savedImgWidth);
                }
            if (!savedImgHeight.empty())
                {
                m_imageHeight = wxAtoi(savedImgHeight);
                }
            }

        // image resize method
        const auto savedResize = barChart->GetPropertyTemplate(L"image-resize-method");
        if (!savedResize.empty())
            {
            const auto method = ReportEnumConvert::ConvertResizeMethod(savedResize);
            if (method.has_value())
                {
                m_imageResizeMethod = method.value();
                }
            }

        // image effect
        const auto savedEffect = barChart->GetPropertyTemplate(L"image-effect");
        if (!savedEffect.empty())
            {
            const auto effect = ReportEnumConvert::ConvertImageEffect(savedEffect);
            if (effect.has_value())
                {
                m_imageEffect = effect.value();
                }
            }

        // image stitch direction
        const auto savedStitch = barChart->GetPropertyTemplate(L"image-stitch");
        if (!savedStitch.empty())
            {
            m_imageStitchDirection = (savedStitch.CmpNoCase(L"vertical") == 0) ?
                                         Orientation::Vertical :
                                         Orientation::Horizontal;
            }

        // color scheme
        m_colorSchemeIndex = ColorSchemeToIndex(barChart->GetColorScheme());

        TransferDataToWindow();

        // update button enabled states after DDX transfers the box effect
        OnBoxEffectChanged();
        }
    } // namespace Wisteria::UI
