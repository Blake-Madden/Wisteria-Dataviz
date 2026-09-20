///////////////////////////////////////////////////////////////////////////////
// Name:        insertpictographdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertpictographdlg.h"
#include "../../app/wisteriaview.h"
#include "../../reporting/reportenumconvert.h"
#include "../variableselectdlg.h"
#include "insertshapedlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertPictographDlg::InsertPictographDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                             wxWindow* parent, const wxString& caption,
                                             const wxWindowID id, const wxPoint& pos,
                                             const wxSize& size, const long style,
                                             EditMode editMode)
        : InsertGraphDlg(
              canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
              static_cast<GraphDlgOptions>(GraphDlgIncludeMost & ~GraphDlgIncludeColorScheme))
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertPictographDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Pictograph"), ID_OPTIONS_SECTION, true);

        // dataset selector
        auto* datasetSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

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

        auto* labelLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Label:"));
        labelLabel->SetFont(labelLabel->GetFont().Bold());
        varGrid->Add(labelLabel, wxSizerFlags{}.CenterVertical());
        m_labelVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_labelVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_labelVarLabel, wxSizerFlags{}.CenterVertical());

        auto* valueLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Value:"));
        valueLabel->SetFont(valueLabel->GetFont().Bold());
        varGrid->Add(valueLabel, wxSizerFlags{}.CenterVertical());
        m_valueVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_valueVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_valueVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // shape button
        auto* shapeBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Icon Shape"));
        auto* shapeButton =
            new wxButton(shapeBox->GetStaticBox(), ID_SELECT_SHAPE_BUTTON, _(L"Select..."));
        shapeBox->Add(shapeButton, wxSizerFlags{}.Border(wxLEFT));

        m_shapeLabel = new wxStaticText(shapeBox->GetStaticBox(), wxID_ANY, wxString{});
        m_shapeLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        shapeBox->Add(m_shapeLabel, wxSizerFlags{}.Border());
        UpdateShapeLabel();

        optionsSizer->Add(shapeBox, wxSizerFlags{}.Border());

        // layout and display options
        auto* layoutSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        layoutSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Orientation:")),
                         wxSizerFlags{}.CenterVertical());
        wxArrayString orientationChoices;
        orientationChoices.Add(_(L"Vertical"));
        orientationChoices.Add(_(L"Horizontal"));
        layoutSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      orientationChoices, 0,
                                      wxGenericValidator{ &m_orientationIndex }));

        layoutSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Value display:")),
                         wxSizerFlags{}.CenterVertical());
        wxArrayString valueFormatChoices;
        valueFormatChoices.Add(_(L"Value"));
        valueFormatChoices.Add(_(L"Percentage"));
        valueFormatChoices.Add(_(L"Currency"));
        layoutSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                      valueFormatChoices, 0,
                                      wxGenericValidator{ &m_valueFormatIndex }));

        layoutSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Fill color:")),
                         wxSizerFlags{}.CenterVertical());
        m_fillColorPicker = new wxColourPickerCtrl(
            optionsPage, wxID_ANY, Graphs::Pictograph::GetDefaultIconBrush().GetColour());
        layoutSizer->Add(m_fillColorPicker);

        layoutSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Outline color:")),
                         wxSizerFlags{}.CenterVertical());
        m_outlineColorPicker = new wxColourPickerCtrl(
            optionsPage, wxID_ANY, Graphs::Pictograph::GetDefaultIconPen().GetColour());
        layoutSizer->Add(m_outlineColorPicker);

        optionsSizer->Add(layoutSizer, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        shapeButton->Bind(wxEVT_BUTTON,
                          [this]([[maybe_unused]] wxCommandEvent&) { OnSelectShape(); });

        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    Orientation InsertPictographDlg::GetOrientation() const noexcept
        {
        return (m_orientationIndex == 1) ? Orientation::Horizontal : Orientation::Vertical;
        }

    //-------------------------------------------
    NumberDisplay InsertPictographDlg::GetValueFormat() const noexcept
        {
        switch (m_valueFormatIndex)
            {
        case 1:
            return NumberDisplay::Percentage;
        case 2:
            return NumberDisplay::Currency;
        default:
            return NumberDisplay::Value;
            }
        }

    //-------------------------------------------
    wxColour InsertPictographDlg::GetFillColor() const
        {
        return (m_fillColorPicker != nullptr) ?
                   m_fillColorPicker->GetColour() :
                   Graphs::Pictograph::GetDefaultIconBrush().GetColour();
        }

    //-------------------------------------------
    wxColour InsertPictographDlg::GetOutlineColor() const
        {
        return (m_outlineColorPicker != nullptr) ?
                   m_outlineColorPicker->GetColour() :
                   Graphs::Pictograph::GetDefaultIconPen().GetColour();
        }

    //-------------------------------------------
    void InsertPictographDlg::OnDatasetChanged()
        {
        m_labelVariable.clear();
        m_valueVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertPictographDlg::OnSelectVariables()
        {
        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset first."), _(L"No Dataset"),
                         wxOK | wxICON_INFORMATION, this);
            return;
            }

        Data::Dataset::ColumnPreviewInfo columnInfo;
        if (GetReportBuilder() != nullptr)
            {
            const auto& importOpts = GetReportBuilder()->GetDatasetImportOptions();
            const int sel = m_datasetChoice->GetSelection();
            if (sel != wxNOT_FOUND && std::cmp_less(sel, m_datasetNames.size()))
                {
                const auto foundPos = importOpts.find(m_datasetNames[sel]);
                if (foundPos != importOpts.cend())
                    {
                    columnInfo = foundPos->second.m_columnPreviewInfo;
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
                  .Label(_(L"Label"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_labelVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_labelVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }),
              VLI{}
                  .Label(_(L"Value"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_valueVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_valueVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto labelVars = dlg.GetSelectedVariables(0);
        m_labelVariable = labelVars.empty() ? wxString{} : labelVars.front();

        const auto valueVars = dlg.GetSelectedVariables(1);
        m_valueVariable = valueVars.empty() ? wxString{} : valueVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertPictographDlg::OnSelectShape()
        {
        InsertShapeDlg dlg(GetCanvas(), GetReportBuilder(), this, _(L"Select Icon Shape"), wxID_ANY,
                           wxDefaultPosition, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                           InsertItemDlg::EditMode::Edit, 0);

        // pre-populate with the previously selected shape
        dlg.SetIconShape(m_iconShape);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        m_iconShape = dlg.GetIconShape();
        UpdateShapeLabel();
        }

    //-------------------------------------------
    void InsertPictographDlg::UpdateVariableLabels()
        {
        m_labelVarLabel->SetLabel(m_labelVariable);
        m_valueVarLabel->SetLabel(m_valueVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    void InsertPictographDlg::UpdateShapeLabel()
        {
        const auto shapeStr = ReportEnumConvert::ConvertIconToString(m_iconShape);
        m_shapeLabel->SetLabel(shapeStr.has_value() ? shapeStr.value() : wxString{});

        if (GetSideBarBook()->GetCurrentPage() != nullptr)
            {
            GetSideBarBook()->GetCurrentPage()->Layout();
            }
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertPictographDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset)
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
    std::shared_ptr<Data::Dataset> InsertPictographDlg::GetSelectedDataset() const
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
        const auto foundPos = datasets.find(m_datasetNames[sel]);
        return (foundPos != datasets.cend()) ? foundPos->second : nullptr;
        }

    //-------------------------------------------
    bool InsertPictographDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_labelVariable.empty() || m_valueVariable.empty())
            {
            wxMessageBox(_(L"Please select the label and value variables."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertPictographDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* pictograph = dynamic_cast<const Graphs::Pictograph*>(&graph);
        if (pictograph == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = pictograph->GetPropertyTemplate(L"dataset");
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

        // load column names from the graph
        m_labelVariable = pictograph->GetLabelColumnName();
        m_valueVariable = pictograph->GetValueColumnName();
        UpdateVariableLabels();

        // icon options
        m_iconShape = pictograph->GetShape();
        UpdateShapeLabel();
        m_orientationIndex = (pictograph->GetOrientation() == Orientation::Horizontal) ? 1 : 0;
        switch (pictograph->GetValueFormat())
            {
        case NumberDisplay::Percentage:
            m_valueFormatIndex = 1;
            break;
        case NumberDisplay::Currency:
            m_valueFormatIndex = 2;
            break;
        default:
            m_valueFormatIndex = 0;
            break;
            }
        m_fillColorPicker->SetColour(pictograph->GetIconBrush().GetColour());
        m_outlineColorPicker->SetColour(pictograph->GetIconPen().GetColour());

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::Pictograph>
    InsertPictographDlg::BuildPictograph(const Graphs::Graph2D* oldGraph)
        {
        auto plot =
            std::make_shared<Graphs::Pictograph>(GetCanvas(), GetIconShape(), GetOrientation());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);

        plot->SetValueFormat(GetValueFormat());

        // when editing, only the colors are changed so that any brush style or pen
        // width/style that was loaded from a project file are preserved
        const auto* oldChart = dynamic_cast<const Graphs::Pictograph*>(oldGraph);
        wxBrush iconBrush{ oldChart != nullptr ? oldChart->GetIconBrush() :
                                                 Graphs::Pictograph::GetDefaultIconBrush() };
        iconBrush.SetColour(GetFillColor());
        plot->SetIconBrush(iconBrush);
        wxPen iconPen{ oldChart != nullptr ? oldChart->GetIconPen() :
                                             Graphs::Pictograph::GetDefaultIconPen() };
        iconPen.SetColour(GetOutlineColor());
        plot->SetIconPen(iconPen);

        plot->SetData(GetSelectedDataset(), GetValueVariable(), GetLabelVariable());

        if (oldGraph != nullptr)
            {
            // carry forward property templates, preserving {{placeholders}}
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.label", GetLabelVariable(),
                oldChart != nullptr ? oldChart->GetLabelColumnName() : wxString{});
            WisteriaView::CarryForwardProperty(
                *oldGraph, *plot, L"variables.value", GetValueVariable(),
                oldChart != nullptr ? oldChart->GetValueColumnName() : wxString{});
            }
        else
            {
            // cache dataset and variable names for round-tripping
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.label", GetLabelVariable());
            plot->SetPropertyTemplate(L"variables.value", GetValueVariable());
            }

        return plot;
        }
    } // namespace Wisteria::UI
