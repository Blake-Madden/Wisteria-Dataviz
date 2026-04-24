///////////////////////////////////////////////////////////////////////////////
// Name:        inserthistogramdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "inserthistogramdlg.h"
#include "../../graphs/histogram.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertHistogramDlg::InsertHistogramDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                           wxWindow* parent, const wxString& caption,
                                           const wxWindowID id, const wxPoint& pos,
                                           const wxSize& size, const long style, EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertHistogramDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();
        CreateAxisOptionsPage();
        CreateAnnotationsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Histogram"), ID_OPTIONS_SECTION, true);

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

        auto* contLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Continuous:"));
        contLabel->SetFont(contLabel->GetFont().Bold());
        varGrid->Add(contLabel, wxSizerFlags{}.CenterVertical());
        m_continuousVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_continuousVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_continuousVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Grouping:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // binning options
        auto* binSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        // binning method
        binSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Binning method:")),
                      wxSizerFlags{}.CenterVertical());
            {
            auto* binMethodChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_binningMethod));
            binMethodChoice->Append(_(L"Unique values"));
            binMethodChoice->Append(_(L"By range"));
            binMethodChoice->Append(_(L"By integer range"));
            binSizer->Add(binMethodChoice);
            }

        // rounding method
        binSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Rounding:")),
                      wxSizerFlags{}.CenterVertical());
            {
            auto* roundChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_roundingMethod));
            roundChoice->Append(_(L"Round"));
            roundChoice->Append(_(L"Round down"));
            roundChoice->Append(_(L"Round up"));
            roundChoice->Append(_(L"No rounding"));
            binSizer->Add(roundChoice);
            }

        // interval display
        binSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Interval display:")),
                      wxSizerFlags{}.CenterVertical());
            {
            auto* intervalChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_intervalDisplay));
            intervalChoice->Append(_(L"Cutpoints"));
            intervalChoice->Append(_(L"Midpoints"));
            binSizer->Add(intervalChoice);
            }

        // bin label display
        binSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Bin labels:")),
                      wxSizerFlags{}.CenterVertical());
            {
            auto* labelChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_binLabelDisplay));
            labelChoice->Append(_(L"Value"));
            labelChoice->Append(_(L"Percentage"));
            labelChoice->Append(_(L"Value & percentage"));
            labelChoice->Append(_(L"No labels"));
            labelChoice->Append(_(L"Name"));
            labelChoice->Append(_(L"Name & value"));
            labelChoice->Append(_(L"Name & percentage"));
            binSizer->Add(labelChoice);
            }

        optionsSizer->Add(binSizer, wxSizerFlags{}.Border());

        // checkboxes
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show full range of values"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showFullRange)),
                          wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Use neat intervals"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_neatIntervals)),
                          wxSizerFlags{}.Border());

        // bin count overrides
        auto* binCountSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        binCountSizer->Add(
            new wxStaticText(optionsPage, wxID_ANY, _(L"Suggested bin count (0 = auto):")),
            wxSizerFlags{}.CenterVertical());
            {
            auto* spin = new wxSpinCtrl(optionsPage, wxID_ANY);
            spin->SetRange(0, 255);
            spin->SetValidator(wxGenericValidator(&m_suggestedBinCount));
            binCountSizer->Add(spin);
            }

        binCountSizer->Add(
            new wxStaticText(optionsPage, wxID_ANY, _(L"Maximum bin count (0 = default):")),
            wxSizerFlags{}.CenterVertical());
            {
            auto* spin = new wxSpinCtrl(optionsPage, wxID_ANY);
            spin->SetRange(0, 255);
            spin->SetValidator(wxGenericValidator(&m_maxBinCount));
            binCountSizer->Add(spin);
            }

        optionsSizer->Add(binCountSizer, wxSizerFlags{}.Border());

        // bin start override
        auto* binStartSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* binStartCheck =
            new wxCheckBox(optionsPage, wxID_ANY, _(L"Start first bin at:"), wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator(&m_overrideBinsStart));
        binStartSizer->Add(binStartCheck, wxSizerFlags{}.CenterVertical());
            {
            m_startBinSpin = new wxSpinCtrlDouble(optionsPage, wxID_ANY);
            m_startBinSpin->SetRange(-1e9, 1e9);
            m_startBinSpin->SetDigits(2);
            m_startBinSpin->SetIncrement(1.0);
            m_startBinSpin->SetValue(m_binsStart);
            m_startBinSpin->Enable(m_overrideBinsStart);
            binStartSizer->Add(m_startBinSpin, wxSizerFlags{}.Border(wxLEFT));
            }
        optionsSizer->Add(binStartSizer, wxSizerFlags{}.Border());

        // showcasing
        auto* ghostBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Showcasing"));

        auto* ghostOpacitySizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        ghostOpacitySizer->Add(
            new wxStaticText(ghostBox->GetStaticBox(), wxID_ANY, _(L"Ghost opacity:")),
            wxSizerFlags{}.CenterVertical());
        auto* opacitySpin = new wxSpinCtrl(ghostBox->GetStaticBox(), wxID_ANY);
        opacitySpin->SetRange(0, 255);
        opacitySpin->SetValidator(wxGenericValidator(&m_ghostOpacity));
        ghostOpacitySizer->Add(opacitySpin);
        ghostBox->Add(ghostOpacitySizer, wxSizerFlags{}.Border());

        m_showcaseListBox = new wxEditableListBox(
            ghostBox->GetStaticBox(), wxID_ANY, _(L"Showcase bins:"), wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(120) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        ghostBox->Add(m_showcaseListBox, wxSizerFlags{ 1 }.Expand().Border());
        optionsSizer->Add(ghostBox, wxSizerFlags{ 1 }.Expand().Border());

        // override New button for showcase bins
        m_showcaseListBox->GetNewButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                const auto dataset = GetSelectedDataset();
                if (dataset == nullptr || m_groupVariable.empty())
                    {
                    wxMessageBox(_(L"Select a grouping variable first to populate available bins."),
                                 _(L"No Groups"), wxOK | wxICON_INFORMATION, this);
                    return;
                    }

                wxArrayString groupChoices;
                const auto groupCol = dataset->GetCategoricalColumn(m_groupVariable);
                if (groupCol != dataset->GetCategoricalColumns().cend())
                    {
                    for (const auto& [id, label] : groupCol->GetStringTable())
                        {
                        if (!label.empty())
                            {
                            groupChoices.Add(label);
                            }
                        }
                    }

                if (groupChoices.empty())
                    {
                    return;
                    }

                wxSingleChoiceDialog dlg(this, _(L"Select bin to showcase:"), _(L"Showcase Bin"),
                                         groupChoices);
                if (dlg.ShowModal() == wxID_OK)
                    {
                    const auto val = dlg.GetStringSelection();
                    if (std::find(m_showcasedBars.begin(), m_showcasedBars.end(), val) ==
                        m_showcasedBars.end())
                        {
                        m_showcasedBars.push_back(val);
                        wxArrayString strings;
                        for (const auto& showBar : m_showcasedBars)
                            {
                            strings.Add(showBar);
                            }
                        m_showcaseListBox->SetStrings(strings);
                        }
                    }
            });

        // override Edit button
        m_showcaseListBox->GetEditButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                const auto dataset = GetSelectedDataset();
                auto* listCtrl = m_showcaseListBox->GetListCtrl();
                const long sel = listCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_showcasedBars.size()) ||
                    dataset == nullptr || m_groupVariable.empty())
                    {
                    return;
                    }

                wxArrayString groupChoices;
                const auto groupCol = dataset->GetCategoricalColumn(m_groupVariable);
                if (groupCol != dataset->GetCategoricalColumns().cend())
                    {
                    for (const auto& [id, label] : groupCol->GetStringTable())
                        {
                        if (!label.empty())
                            {
                            groupChoices.Add(label);
                            }
                        }
                    }

                if (groupChoices.empty())
                    {
                    return;
                    }

                wxSingleChoiceDialog dlg(this, _(L"Select bin to showcase:"), _(L"Showcase Bin"),
                                         groupChoices);
                dlg.SetSelection(sel);
                if (dlg.ShowModal() == wxID_OK)
                    {
                    m_showcasedBars[sel] = dlg.GetStringSelection();
                    wxArrayString strings;
                    for (const auto& s : m_showcasedBars)
                        {
                        strings.Add(s);
                        }
                    m_showcaseListBox->SetStrings(strings);
                    }
            });

        // override Delete button
        m_showcaseListBox->GetDelButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                const long sel = m_showcaseListBox->GetListCtrl()->GetNextItem(
                    -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_showcasedBars.size()))
                    {
                    return;
                    }
                m_showcasedBars.erase(m_showcasedBars.begin() + sel);
                wxArrayString strings;
                for (const auto& s : m_showcasedBars)
                    {
                    strings.Add(s);
                    }
                m_showcaseListBox->SetStrings(strings);
            });

        // legend placement
        auto* legendSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        legendSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Legend:")),
                         wxSizerFlags{}.CenterVertical());
        legendSizer->Add(CreateLegendPlacementChoice(optionsPage, 1));
        optionsSizer->Add(legendSizer, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        binStartCheck->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& evt)
                            { m_startBinSpin->Enable(evt.IsChecked()); });
        }

    //-------------------------------------------
    void InsertHistogramDlg::OnDatasetChanged()
        {
        m_continuousVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertHistogramDlg::OnSelectVariables()
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
                  .Label(_(L"Continuous"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_continuousVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_continuousVariable })
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

        const auto contVars = dlg.GetSelectedVariables(0);
        m_continuousVariable = contVars.empty() ? wxString{} : contVars.front();

        const auto groupVars = dlg.GetSelectedVariables(1);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertHistogramDlg::UpdateVariableLabels()
        {
        m_continuousVarLabel->SetLabel(m_continuousVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        // clear showcase if group changes
        m_showcasedBars.clear();
        if (m_showcaseListBox != nullptr)
            {
            m_showcaseListBox->SetStrings(wxArrayString{});
            }

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertHistogramDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertHistogramDlg::GetSelectedDataset() const
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
    bool InsertHistogramDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_continuousVariable.empty())
            {
            wxMessageBox(_(L"Please select the continuous variable."), _(L"Variable Not Specified"),
                         wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        if (!ValidateColorScheme())
            {
            return false;
            }

        // validators do not work with wxSpinCtrlDouble
        m_binsStart = m_startBinSpin->GetValue();

        return true;
        }

    //-------------------------------------------
    void InsertHistogramDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* histogram = dynamic_cast<const Graphs::Histogram*>(&graph);
        if (histogram == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = histogram->GetPropertyTemplate(L"dataset");
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
        m_continuousVariable = histogram->GetContinuousColumnName();
        m_groupVariable = histogram->GetGroupColumnName().value_or(wxString{});
        UpdateVariableLabels();

        // binning options
        m_binningMethod = static_cast<int>(histogram->GetBinningMethod());
        m_roundingMethod = static_cast<int>(histogram->GetRoundingMethod());
        m_intervalDisplay = static_cast<int>(histogram->GetIntervalDisplay());
        m_binLabelDisplay = static_cast<int>(histogram->GetBinLabelDisplay());
        m_showFullRange = histogram->IsShowingFullRangeOfValues();
        m_neatIntervals = histogram->IsUsingNeatIntervals();
        m_ghostOpacity = histogram->GetGhostOpacity();
        m_suggestedBinCount = histogram->GetSuggestedBinCount().has_value() ?
                                  static_cast<int>(histogram->GetSuggestedBinCount().value()) :
                                  0;
        m_maxBinCount = (histogram->GetMaxNumberOfBins() != 255) ?
                            static_cast<int>(histogram->GetMaxNumberOfBins()) :
                            0;
        m_overrideBinsStart = histogram->GetBinsStart().has_value();
        m_binsStart = histogram->GetBinsStart().value_or(0.0);
        m_showcasedBars = histogram->GetShowcasedLabels();
        if (m_showcaseListBox != nullptr)
            {
            wxArrayString strings;
            for (const auto& showBar : m_showcasedBars)
                {
                strings.Add(showBar);
                }
            m_showcaseListBox->SetStrings(strings);
            }

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
