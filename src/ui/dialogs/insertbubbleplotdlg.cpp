///////////////////////////////////////////////////////////////////////////////
// Name:        insertbubbleplotdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertbubbleplotdlg.h"
#include "../../graphs/bubbleplot.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertBubblePlotDlg::InsertBubblePlotDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
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
    void InsertBubblePlotDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Bubble Plot Options"), ID_OPTIONS_SECTION, true);

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

        auto* xLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"X (independent):"));
        xLabel->SetFont(xLabel->GetFont().Bold());
        varGrid->Add(xLabel, wxSizerFlags{}.CenterVertical());
        m_xVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_xVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_xVarLabel, wxSizerFlags{}.CenterVertical());

        auto* yLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Y (dependent):"));
        yLabel->SetFont(yLabel->GetFont().Bold());
        varGrid->Add(yLabel, wxSizerFlags{}.CenterVertical());
        m_yVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_yVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_yVarLabel, wxSizerFlags{}.CenterVertical());

        auto* sizeLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Size (area):"));
        sizeLabel->SetFont(sizeLabel->GetFont().Bold());
        varGrid->Add(sizeLabel, wxSizerFlags{}.CenterVertical());
        m_sizeVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_sizeVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_sizeVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Grouping:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(GetVariableLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // regression options
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show regression lines"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showRegressionLines)),
                          wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show confidence bands"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showConfidenceBands)),
                          wxSizerFlags{}.Border());

        // bubble sizing
        auto* bubbleSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        bubbleSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Min bubble radius:")),
                         wxSizerFlags{}.CenterVertical());
            {
            auto* spin = new wxSpinCtrl(optionsPage, wxID_ANY);
            spin->SetRange(1, 100);
            spin->SetValue(m_minBubbleRadius);
            spin->SetValidator(wxGenericValidator(&m_minBubbleRadius));
            bubbleSizer->Add(spin);
            }

        bubbleSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Max bubble radius:")),
                         wxSizerFlags{}.CenterVertical());
            {
            auto* spin = new wxSpinCtrl(optionsPage, wxID_ANY);
            spin->SetRange(1, 200);
            spin->SetValue(m_maxBubbleRadius);
            spin->SetValidator(wxGenericValidator(&m_maxBubbleRadius));
            bubbleSizer->Add(spin);
            }

        optionsSizer->Add(bubbleSizer, wxSizerFlags{}.Border());

        // color scheme
        auto* colorSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        colorSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Color scheme:")),
                        wxSizerFlags{}.CenterVertical());
        colorSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     GetColorSchemeNames(), 0,
                                     wxGenericValidator(&m_colorSchemeIndex)),
                        wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(colorSizer, wxSizerFlags{}.Border());

        // shape scheme
        auto* shapeSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        shapeSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Point shapes:")),
                        wxSizerFlags{}.CenterVertical());
        wxArrayString shapeSchemes;
        shapeSchemes.Add(_(L"Standard Shapes"));
        shapeSchemes.Add(_(L"Semesters"));
        shapeSizer->Add(new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                     shapeSchemes, 0, wxGenericValidator(&m_shapeSchemeIndex)),
                        wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(shapeSizer, wxSizerFlags{}.Border());

        // legend placement
        auto* legendSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        legendSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Legend:")),
                         wxSizerFlags{}.CenterVertical());
        legendSizer->Add(CreateLegendPlacementChoice(optionsPage, 1));
        optionsSizer->Add(legendSizer, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });
        }

    //-------------------------------------------
    void InsertBubblePlotDlg::OnDatasetChanged()
        {
        m_xVariable.clear();
        m_yVariable.clear();
        m_sizeVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertBubblePlotDlg::OnSelectVariables()
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
                  .Label(_(L"X (independent)"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_xVariable.empty() ? std::vector<wxString>{} :
                                                          std::vector<wxString>{ m_xVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Y (dependent)"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_yVariable.empty() ? std::vector<wxString>{} :
                                                          std::vector<wxString>{ m_yVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Size (area)"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_sizeVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_sizeVariable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Grouping"))
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

        const auto xVars = dlg.GetSelectedVariables(0);
        m_xVariable = xVars.empty() ? wxString{} : xVars.front();

        const auto yVars = dlg.GetSelectedVariables(1);
        m_yVariable = yVars.empty() ? wxString{} : yVars.front();

        const auto sizeVars = dlg.GetSelectedVariables(2);
        m_sizeVariable = sizeVars.empty() ? wxString{} : sizeVars.front();

        const auto groupVars = dlg.GetSelectedVariables(3);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertBubblePlotDlg::UpdateVariableLabels()
        {
        m_xVarLabel->SetLabel(m_xVariable);
        m_yVarLabel->SetLabel(m_yVariable);
        m_sizeVarLabel->SetLabel(m_sizeVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertBubblePlotDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertBubblePlotDlg::GetSelectedDataset() const
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
    bool InsertBubblePlotDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_xVariable.empty() || m_yVariable.empty() || m_sizeVariable.empty())
            {
            wxMessageBox(_(L"Please select the X, Y, and size variables."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertBubblePlotDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* bubble = dynamic_cast<const Graphs::BubblePlot*>(&graph);
        if (bubble == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = bubble->GetPropertyTemplate(L"dataset");
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

        // load the actual column names used by the graph
        m_xVariable = bubble->GetXColumnName();
        m_yVariable = bubble->GetYColumnName();
        m_sizeVariable = bubble->GetSizeColumnName();
        const auto& series = bubble->GetSeriesList();
        if (!series.empty())
            {
            m_groupVariable = series.front().GetGroupColumnName().value_or(wxString{});
            }
        UpdateVariableLabels();

        // bubble-specific options
        m_showRegressionLines = bubble->IsShowingRegressionLines();
        m_showConfidenceBands = bubble->IsShowingConfidenceBands();
        m_minBubbleRadius = static_cast<int>(bubble->GetMinBubbleRadius());
        m_maxBubbleRadius = static_cast<int>(bubble->GetMaxBubbleRadius());

        // determine which color scheme is in use
        m_colorSchemeIndex = ColorSchemeToIndex(bubble->GetColorScheme());

        // determine which shape scheme is in use
        const auto& scheme = bubble->GetShapeScheme();
        if (scheme != nullptr && scheme->IsKindOf(wxCLASSINFO(Icons::Schemes::Semesters)))
            {
            m_shapeSchemeIndex = 1;
            }
        else
            {
            m_shapeSchemeIndex = 0;
            }

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
