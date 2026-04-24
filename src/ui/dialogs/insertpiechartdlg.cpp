///////////////////////////////////////////////////////////////////////////////
// Name:        insertpiechartdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertpiechartdlg.h"
#include "../../graphs/piechart.h"
#include "insertlabeldlg.h"
#include "variableselectdlg.h"
#include <wx/clrpicker.h>
#include <wx/gbsizer.h>
#include <wx/spinctrl.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertPieChartDlg::InsertPieChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
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
    void InsertPieChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxGridBagSizer(wxSizerFlags::GetDefaultBorder(),
                                                wxSizerFlags::GetDefaultBorder() * 2);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Pie Chart"), ID_OPTIONS_SECTION, true);

        auto* leftColumnSizer = new wxBoxSizer(wxVERTICAL);
        auto* rightColumnSizer = new wxBoxSizer(wxVERTICAL);
        optionsSizer->Add(leftColumnSizer, wxGBPosition(0, 0), wxGBSpan(1, 1), wxEXPAND);
        optionsSizer->Add(rightColumnSizer, wxGBPosition(0, 1), wxGBSpan(1, 1), wxEXPAND);
        optionsSizer->AddGrowableCol(0, 1);
        optionsSizer->AddGrowableCol(1, 1);
        optionsSizer->AddGrowableRow(0, 1);

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

        leftColumnSizer->Add(datasetSizer, wxSizerFlags{}.Border());

        // variables button
        auto* varsBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Variables"));
        auto* varButton =
            new wxButton(varsBox->GetStaticBox(), ID_SELECT_VARS_BUTTON, _(L"Select..."));
        varsBox->Add(varButton, wxSizerFlags{}.Border(wxLEFT));

        // variable label grid
        auto* varGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(12), FromDIP(2) });

        auto* groupLabel =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Group (outer):"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        auto* weightLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Weight:"));
        weightLabel->SetFont(weightLabel->GetFont().Bold());
        varGrid->Add(weightLabel, wxSizerFlags{}.CenterVertical());
        m_weightVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_weightVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_weightVarLabel, wxSizerFlags{}.CenterVertical());

        auto* group2Label =
            new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Subgroup (inner):"));
        group2Label->SetFont(group2Label->GetFont().Bold());
        varGrid->Add(group2Label, wxSizerFlags{}.CenterVertical());
        m_group2VarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_group2VarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_group2VarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        leftColumnSizer->Add(varsBox, wxSizerFlags{}.Border().Expand());

        // pie style
        auto* styleSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        styleSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Style:")),
                        wxSizerFlags{}.CenterVertical());
            {
            auto* styleChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_pieStyle));
            styleChoice->Append(_(L"None"));
            styleChoice->Append(_(L"Clockface"));
            styleChoice->Append(_(L"Cheese pizza"));
            styleChoice->Append(_(L"Pepperoni pizza"));
            styleChoice->Append(_(L"Hawaiian pizza"));
            styleChoice->Append(_(L"Coffee ring"));
            styleChoice->Append(_(L"Venus"));
            styleChoice->Append(_(L"Mars"));
            styleChoice->Append(_(L"Chocolate chip cookie"));
            styleSizer->Add(styleChoice);
            }
        leftColumnSizer->Add(styleSizer, wxSizerFlags{}.Border());

        // labels area
        auto* labelsBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Labels"));

        auto* labelGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        // outer midpoint labels
        labelGrid->Add(new wxStaticText(labelsBox->GetStaticBox(), wxID_ANY, _(L"Outer midpoint:")),
                       wxSizerFlags{}.CenterVertical());
            {
            auto* midPointChoice =
                new wxChoice(labelsBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator(&m_outerMidPointDisplay));
            midPointChoice->Append(_(L"Value"));
            midPointChoice->Append(_(L"Percentage"));
            midPointChoice->Append(_(L"Value & percentage"));
            midPointChoice->Append(_(L"No labels"));
            midPointChoice->Append(_(L"Name"));
            midPointChoice->Append(_(L"Name & value"));
            midPointChoice->Append(_(L"Name & percentage"));
            labelGrid->Add(midPointChoice);
            }

        // outer labels (gutter)
        labelGrid->Add(new wxStaticText(labelsBox->GetStaticBox(), wxID_ANY, _(L"Outer:")),
                       wxSizerFlags{}.CenterVertical());
            {
            auto* outerLabelChoice =
                new wxChoice(labelsBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator(&m_outerLabelDisplay));
            outerLabelChoice->Append(_(L"Value"));
            outerLabelChoice->Append(_(L"Percentage"));
            outerLabelChoice->Append(_(L"Value & percentage"));
            outerLabelChoice->Append(_(L"No labels"));
            outerLabelChoice->Append(_(L"Name"));
            outerLabelChoice->Append(_(L"Name & value"));
            outerLabelChoice->Append(_(L"Name & percentage"));
            labelGrid->Add(outerLabelChoice);
            }

        // inner midpoint labels
        labelGrid->Add(new wxStaticText(labelsBox->GetStaticBox(), wxID_ANY, _(L"Inner midpoint:")),
                       wxSizerFlags{}.CenterVertical());
            {
            auto* innerMidChoice =
                new wxChoice(labelsBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator(&m_innerMidPointDisplay));
            innerMidChoice->Append(_(L"Value"));
            innerMidChoice->Append(_(L"Percentage"));
            innerMidChoice->Append(_(L"Value & percentage"));
            innerMidChoice->Append(_(L"No labels"));
            innerMidChoice->Append(_(L"Name"));
            innerMidChoice->Append(_(L"Name & value"));
            innerMidChoice->Append(_(L"Name & percentage"));
            labelGrid->Add(innerMidChoice);
            }

        // label placement
        labelGrid->Add(new wxStaticText(labelsBox->GetStaticBox(), wxID_ANY, _(L"Placement:")),
                       wxSizerFlags{}.CenterVertical());
            {
            auto* placementChoice =
                new wxChoice(labelsBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator(&m_labelPlacement));
            placementChoice->Append(_(L"Next to slice"));
            placementChoice->Append(_(L"Flush"));
            labelGrid->Add(placementChoice);
            }

        labelsBox->Add(labelGrid, wxSizerFlags{}.Border());

        labelsBox->Add(new wxCheckBox(labelsBox->GetStaticBox(), wxID_ANY,
                                      _(L"Show outer pie labels"), wxDefaultPosition, wxDefaultSize,
                                      0, wxGenericValidator(&m_showOuterPieLabels)),
                       wxSizerFlags{}.Border());

        labelsBox->Add(new wxCheckBox(labelsBox->GetStaticBox(), wxID_ANY,
                                      _(L"Show inner pie labels"), wxDefaultPosition, wxDefaultSize,
                                      0, wxGenericValidator(&m_showInnerPieLabels)),
                       wxSizerFlags{}.Border());

        labelsBox->Add(new wxCheckBox(labelsBox->GetStaticBox(), wxID_ANY,
                                      _(L"Color-matched labels"), wxDefaultPosition, wxDefaultSize,
                                      0, wxGenericValidator(&m_useColorLabels)),
                       wxSizerFlags{}.Border());

        leftColumnSizer->Add(labelsBox, wxSizerFlags{}.Border().Expand());

        // donut hole area
        auto* donutBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Donut Hole"));

        auto* donutCheckBox = new wxCheckBox(
            donutBox->GetStaticBox(), wxID_ANY, _(L"Include donut hole"), wxDefaultPosition,
            wxDefaultSize, 0, wxGenericValidator(&m_includeDonutHole));
        donutBox->Add(donutCheckBox, wxSizerFlags{}.Border());

        auto* donutGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        m_donutColorLabel =
            new wxStaticText(donutBox->GetStaticBox(), wxID_ANY, _(L"Background color:"));
        donutGrid->Add(m_donutColorLabel, wxSizerFlags{}.CenterVertical());

        auto* donutColorPicker = new wxColourPickerCtrl(
            donutBox->GetStaticBox(), wxID_ANY, *wxWHITE, wxDefaultPosition, wxDefaultSize,
            wxCLRP_DEFAULT_STYLE, wxGenericValidator(&m_donutHoleColor));
        m_donutColorPicker = donutColorPicker;
        donutGrid->Add(donutColorPicker, wxSizerFlags{}.CenterVertical());

        m_donutProportionLabel =
            new wxStaticText(donutBox->GetStaticBox(), wxID_ANY, _(L"Hole proportion:"));
        donutGrid->Add(m_donutProportionLabel, wxSizerFlags{}.CenterVertical());

        auto* proportionSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* donutProportionSpin = new wxSpinCtrl(donutBox->GetStaticBox(), wxID_ANY);
        donutProportionSpin->SetRange(0, 100);
        donutProportionSpin->SetValidator(wxGenericValidator(&m_donutHoleProportion));
        m_donutProportionSpin = donutProportionSpin;
        proportionSizer->Add(donutProportionSpin, wxSizerFlags{}.CenterVertical());
        m_donutProportionPercentLabel = new wxStaticText(donutBox->GetStaticBox(), wxID_ANY, L"%");
        proportionSizer->Add(m_donutProportionPercentLabel,
                             wxSizerFlags{}.CenterVertical().Border(wxLEFT));
        donutGrid->Add(proportionSizer, wxSizerFlags{}.CenterVertical());

        donutBox->Add(donutGrid, wxSizerFlags{}.Border(wxLEFT));

        m_editDonutLabelButton =
            new wxButton(donutBox->GetStaticBox(), wxID_ANY, _(L"Edit Label..."));
        m_editDonutLabelButton->Enable(m_includeDonutHole);
        donutBox->Add(m_editDonutLabelButton, wxSizerFlags{}.Border(wxLEFT | wxTOP | wxBOTTOM));

        // showcasing
        auto* showcaseBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Showcasing"));

        auto* showcaseGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        showcaseGrid->Add(
            new wxStaticText(showcaseBox->GetStaticBox(), wxID_ANY, _(L"Ghost opacity:")),
            wxSizerFlags{}.CenterVertical());
        auto* opacitySpin = new wxSpinCtrl(showcaseBox->GetStaticBox(), wxID_ANY);
        opacitySpin->SetRange(0, 255);
        opacitySpin->SetValidator(wxGenericValidator(&m_ghostOpacity));
        showcaseGrid->Add(opacitySpin);

        showcaseGrid->Add(new wxStaticText(showcaseBox->GetStaticBox(), wxID_ANY, _(L"Mode:")),
                          wxSizerFlags{}.CenterVertical());
        m_showcaseModeChoice =
            new wxChoice(showcaseBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                         nullptr, 0, wxGenericValidator(&m_showcaseMode));
        m_showcaseModeChoice->Append(_(L"None"));
        m_showcaseModeChoice->Append(_(L"Explicit list of outer slices"));
        m_showcaseModeChoice->Append(_(L"Largest outer slice(s)"));
        m_showcaseModeChoice->Append(_(L"Smallest outer slice(s)"));
        m_showcaseModeChoice->Append(_(L"Largest inner slice(s)"));
        m_showcaseModeChoice->Append(_(L"Smallest inner slice(s)"));
        showcaseGrid->Add(m_showcaseModeChoice);

        showcaseGrid->Add(
            new wxStaticText(showcaseBox->GetStaticBox(), wxID_ANY, _(L"Ring labels shown:")),
            wxSizerFlags{}.CenterVertical());
        m_showcasedRingChoice =
            new wxChoice(showcaseBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                         nullptr, 0, wxGenericValidator(&m_showcasedRingLabels));
        m_showcasedRingChoice->Append(_(L"Inner"));
        m_showcasedRingChoice->Append(_(L"Outer"));
        showcaseGrid->Add(m_showcasedRingChoice);
        showcaseBox->Add(showcaseGrid, wxSizerFlags{}.Border());

        m_showcaseByGroupCheck = new wxCheckBox(
            showcaseBox->GetStaticBox(), wxID_ANY, _(L"Group by outer slice"), wxDefaultPosition,
            wxDefaultSize, 0, wxGenericValidator(&m_showcaseByGroup));
        showcaseBox->Add(m_showcaseByGroupCheck, wxSizerFlags{}.Border());

        m_showcaseShowOuterMidPtsCheck = new wxCheckBox(
            showcaseBox->GetStaticBox(), wxID_ANY,
            _(L"Show outer pie midpoint labels while showcasing"), wxDefaultPosition, wxDefaultSize,
            0, wxGenericValidator(&m_showcaseShowOuterPieMidPointLabels));
        showcaseBox->Add(m_showcaseShowOuterMidPtsCheck, wxSizerFlags{}.Border());

        m_showcaseListBox = new wxEditableListBox(
            showcaseBox->GetStaticBox(), wxID_ANY, _(L"Slices to showcase:"), wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(120) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        showcaseBox->Add(m_showcaseListBox, wxSizerFlags{ 1 }.Expand().Border());

        // override New button for showcase slices
        m_showcaseListBox->GetNewButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                if (GetSelectedDataset() == nullptr || m_groupVariable.empty())
                    {
                    wxMessageBox(
                        _(L"Select an outer grouping variable first to populate available slices."),
                        _(L"No Groups"), wxOK | wxICON_INFORMATION, this);
                    return;
                    }
                const auto sliceChoices = GetOuterSliceChoices();
                if (sliceChoices.empty())
                    {
                    return;
                    }
                wxSingleChoiceDialog dlg(this, _(L"Select slice to showcase:"),
                                         _(L"Showcase Slice"), sliceChoices);
                if (dlg.ShowModal() == wxID_OK)
                    {
                    const auto val = dlg.GetStringSelection();
                    if (std::find(m_showcaseSlices.cbegin(), m_showcaseSlices.cend(), val) ==
                        m_showcaseSlices.cend())
                        {
                        m_showcaseSlices.push_back(val);
                        RefreshShowcaseListBox();
                        }
                    }
            });

        // override Edit button
        m_showcaseListBox->GetEditButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                auto* listCtrl = m_showcaseListBox->GetListCtrl();
                const long sel = listCtrl->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_showcaseSlices.size()) ||
                    GetSelectedDataset() == nullptr || m_groupVariable.empty())
                    {
                    return;
                    }
                const auto sliceChoices = GetOuterSliceChoices();
                if (sliceChoices.empty())
                    {
                    return;
                    }
                wxSingleChoiceDialog dlg(this, _(L"Select slice to showcase:"),
                                         _(L"Showcase Slice"), sliceChoices);
                const int found = sliceChoices.Index(m_showcaseSlices[sel]);
                if (found != wxNOT_FOUND)
                    {
                    dlg.SetSelection(found);
                    }
                if (dlg.ShowModal() == wxID_OK)
                    {
                    m_showcaseSlices[sel] = dlg.GetStringSelection();
                    RefreshShowcaseListBox();
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
                if (sel < 0 || std::cmp_greater_equal(sel, m_showcaseSlices.size()))
                    {
                    return;
                    }
                m_showcaseSlices.erase(std::next(m_showcaseSlices.cbegin(), sel));
                RefreshShowcaseListBox();
            });

        m_showcaseModeChoice->Bind(wxEVT_CHOICE, [this]([[maybe_unused]] wxCommandEvent&)
                                   { OnShowcaseModeChanged(); });

        rightColumnSizer->Add(showcaseBox, wxSizerFlags{ 1 }.Expand().Border());
        rightColumnSizer->Add(donutBox, wxSizerFlags{}.Border().Expand());

        // legend placement (at the bottom of the left column)
        auto* legendSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        legendSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Legend:")),
                         wxSizerFlags{}.CenterVertical());
        legendSizer->Add(CreateLegendPlacementChoice(optionsPage, 1));
        leftColumnSizer->AddStretchSpacer(1);
        leftColumnSizer->Add(legendSizer, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        donutCheckBox->Bind(wxEVT_CHECKBOX,
                            [this]([[maybe_unused]] wxCommandEvent&)
                            {
                                TransferDataFromWindow();
                                m_editDonutLabelButton->Enable(m_includeDonutHole);
                                if (m_donutColorPicker != nullptr)
                                    {
                                    m_donutColorPicker->Enable(m_includeDonutHole);
                                    }
                                if (m_donutColorLabel != nullptr)
                                    {
                                    m_donutColorLabel->Enable(m_includeDonutHole);
                                    }
                                if (m_donutProportionSpin != nullptr)
                                    {
                                    m_donutProportionSpin->Enable(m_includeDonutHole);
                                    }
                                if (m_donutProportionLabel != nullptr)
                                    {
                                    m_donutProportionLabel->Enable(m_includeDonutHole);
                                    }
                                if (m_donutProportionPercentLabel != nullptr)
                                    {
                                    m_donutProportionPercentLabel->Enable(m_includeDonutHole);
                                    }
                            });

        if (m_donutColorPicker != nullptr)
            {
            m_donutColorPicker->Enable(m_includeDonutHole);
            }
        if (m_donutColorLabel != nullptr)
            {
            m_donutColorLabel->Enable(m_includeDonutHole);
            }
        if (m_donutProportionSpin != nullptr)
            {
            m_donutProportionSpin->Enable(m_includeDonutHole);
            }
        if (m_donutProportionLabel != nullptr)
            {
            m_donutProportionLabel->Enable(m_includeDonutHole);
            }
        if (m_donutProportionPercentLabel != nullptr)
            {
            m_donutProportionPercentLabel->Enable(m_includeDonutHole);
            }

        m_editDonutLabelButton->Bind(wxEVT_BUTTON, [this]([[maybe_unused]] wxCommandEvent&)
                                     { OnEditDonutHoleLabel(); });

        OnShowcaseModeChanged();
        }

    //-------------------------------------------
    void InsertPieChartDlg::OnShowcaseModeChanged()
        {
        TransferDataFromWindow();
        using SM = Wisteria::Graphs::PieChart::ShowcaseMode;
        const auto mode = static_cast<SM>(m_showcaseMode);
        const bool isInner = (mode == SM::LargestInner || mode == SM::SmallestInner);
        const bool isExplicit = (mode == SM::ExplicitList);
        const bool isOuterMode =
            (mode == SM::LargestOuter || mode == SM::SmallestOuter || isExplicit);

        if (m_showcasedRingChoice != nullptr)
            {
            m_showcasedRingChoice->Enable(isOuterMode);
            }
        if (m_showcaseByGroupCheck != nullptr)
            {
            m_showcaseByGroupCheck->Enable(isInner);
            }
        if (m_showcaseShowOuterMidPtsCheck != nullptr)
            {
            m_showcaseShowOuterMidPtsCheck->Enable(isInner);
            }
        if (m_showcaseListBox != nullptr)
            {
            m_showcaseListBox->Enable(isExplicit);
            }
        }

    //-------------------------------------------
    void InsertPieChartDlg::RefreshShowcaseListBox()
        {
        if (m_showcaseListBox == nullptr)
            {
            return;
            }
        wxArrayString strings;
        for (const auto& slice : m_showcaseSlices)
            {
            strings.Add(slice);
            }
        m_showcaseListBox->SetStrings(strings);
        }

    //-------------------------------------------
    wxArrayString InsertPieChartDlg::GetOuterSliceChoices() const
        {
        wxArrayString choices;
        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr || m_groupVariable.empty())
            {
            return choices;
            }
        const auto col = dataset->GetCategoricalColumn(m_groupVariable);
        if (col != dataset->GetCategoricalColumns().cend())
            {
            for (const auto& [id, label] : col->GetStringTable())
                {
                if (!label.empty())
                    {
                    choices.Add(label);
                    }
                }
            }
        return choices;
        }

    //-------------------------------------------
    void InsertPieChartDlg::OnDatasetChanged()
        {
        m_groupVariable.clear();
        m_weightVariable.clear();
        m_group2Variable.clear();
        m_showcaseSlices.clear();
        RefreshShowcaseListBox();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertPieChartDlg::OnSelectVariables()
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
                  .Label(_(L"Group (outer)"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_groupVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_groupVariable })
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
                  .Label(_(L"Subgroup (inner)"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(m_group2Variable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_group2Variable })
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::String,
                                   Data::Dataset::ColumnImportType::Discrete,
                                   Data::Dataset::ColumnImportType::DichotomousString,
                                   Data::Dataset::ColumnImportType::DichotomousDiscrete }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto groupVars = dlg.GetSelectedVariables(0);
        const auto newGroup = groupVars.empty() ? wxString{} : groupVars.front();
        if (newGroup != m_groupVariable)
            {
            m_showcaseSlices.clear();
            RefreshShowcaseListBox();
            }
        m_groupVariable = newGroup;

        const auto weightVars = dlg.GetSelectedVariables(1);
        m_weightVariable = weightVars.empty() ? wxString{} : weightVars.front();

        const auto group2Vars = dlg.GetSelectedVariables(2);
        m_group2Variable = group2Vars.empty() ? wxString{} : group2Vars.front();

        UpdateVariableLabels();
        OnShowcaseModeChanged();
        }

    //-------------------------------------------
    void InsertPieChartDlg::UpdateVariableLabels()
        {
        m_groupVarLabel->SetLabel(m_groupVariable);
        m_weightVarLabel->SetLabel(m_weightVariable);
        m_group2VarLabel->SetLabel(m_group2Variable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertPieChartDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertPieChartDlg::GetSelectedDataset() const
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
    bool InsertPieChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_groupVariable.empty())
            {
            wxMessageBox(_(L"Please select the grouping variable."), _(L"Variable Not Specified"),
                         wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        using SM = Wisteria::Graphs::PieChart::ShowcaseMode;
        const auto mode = static_cast<SM>(m_showcaseMode);
        if ((mode == SM::LargestInner || mode == SM::SmallestInner) && m_group2Variable.empty())
            {
            wxMessageBox(_(L"Inner showcase modes require an inner subgroup variable."),
                         _(L"Subgroup Required"), wxOK | wxICON_WARNING, this);
            return false;
            }

        if (mode == SM::ExplicitList && m_showcaseSlices.empty())
            {
            wxMessageBox(_(L"Add at least one slice to showcase, or change the showcase mode."),
                         _(L"No Slices Specified"), wxOK | wxICON_WARNING, this);
            return false;
            }

        if (!ValidateColorScheme())
            {
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertPieChartDlg::OnEditDonutHoleLabel()
        {
        InsertLabelDlg dlg(GetCanvas(), nullptr, this, _(L"Edit Donut Hole Label"), wxID_ANY,
                           wxDefaultPosition, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                           EditMode::Edit, false);
        dlg.LoadFromLabel(m_donutHoleLabel);

        if (dlg.ShowModal() == wxID_OK)
            {
            dlg.ApplyToLabel(m_donutHoleLabel);
            }
        }

    //-------------------------------------------
    void InsertPieChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* pieChart = dynamic_cast<const Graphs::PieChart*>(&graph);
        if (pieChart == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = pieChart->GetPropertyTemplate(L"dataset");
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
        m_groupVariable = pieChart->GetGroupColumn1Name();
        m_weightVariable = pieChart->GetWeightColumnName();
        m_group2Variable = pieChart->GetGroupColumn2Name();
        UpdateVariableLabels();

        m_includeDonutHole = pieChart->IsIncludingDonutHole();
        m_showOuterPieLabels = pieChart->IsShowingOuterPieLabels();
        m_showInnerPieLabels = pieChart->IsShowingInnerPieLabels();
        m_useColorLabels = pieChart->IsUsingColorLabels();
        m_outerMidPointDisplay = static_cast<int>(pieChart->GetOuterPieMidPointLabelDisplay());
        m_outerLabelDisplay = static_cast<int>(pieChart->GetOuterLabelDisplay());
        m_innerMidPointDisplay = static_cast<int>(pieChart->GetInnerPieMidPointLabelDisplay());
        m_labelPlacement = static_cast<int>(pieChart->GetLabelPlacement());
        m_pieStyle = static_cast<int>(pieChart->GetPieStyle());

        m_donutHoleLabel = pieChart->GetDonutHoleLabel();
        m_donutHoleColor = pieChart->GetDonutHoleColor();
        m_donutHoleProportion =
            static_cast<int>(std::lround(pieChart->GetDonutHoleProportion() * 100));

        m_showcaseMode = static_cast<int>(pieChart->GetShowcaseMode());
        m_showcasedRingLabels = static_cast<int>(pieChart->GetShowcasedRingLabels());
        m_showcaseByGroup = pieChart->IsShowcaseByGroup();
        m_showcaseShowOuterPieMidPointLabels = pieChart->IsShowcaseShowingOuterPieMidPointLabels();
        m_ghostOpacity = static_cast<int>(pieChart->GetGhostOpacity());

        m_showcaseSlices.clear();
        if (pieChart->GetShowcaseMode() == Wisteria::Graphs::PieChart::ShowcaseMode::ExplicitList)
            {
            for (size_t i = 0;; ++i)
                {
                const auto val =
                    pieChart->GetPropertyTemplate(L"showcase-slices[" + std::to_wstring(i) + L"]");
                if (val.empty())
                    {
                    break;
                    }
                m_showcaseSlices.push_back(val);
                }
            }
        RefreshShowcaseListBox();

        TransferDataToWindow();
        m_editDonutLabelButton->Enable(m_includeDonutHole);
        if (m_donutColorLabel != nullptr)
            {
            m_donutColorLabel->Enable(m_includeDonutHole);
            }
        if (m_donutColorPicker != nullptr)
            {
            m_donutColorPicker->Enable(m_includeDonutHole);
            }
        if (m_donutProportionSpin != nullptr)
            {
            m_donutProportionSpin->Enable(m_includeDonutHole);
            }
        if (m_donutProportionLabel != nullptr)
            {
            m_donutProportionLabel->Enable(m_includeDonutHole);
            }
        if (m_donutProportionPercentLabel != nullptr)
            {
            m_donutProportionPercentLabel->Enable(m_includeDonutHole);
            }
        OnShowcaseModeChanged();
        }
    } // namespace Wisteria::UI
