///////////////////////////////////////////////////////////////////////////////
// Name:        insertlineplotdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertlineplotdlg.h"
#include "../../app/wisteriaview.h"
#include "../../graphs/lineplot.h"
#include "../../util/donttranslate.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertLinePlotDlg::InsertLinePlotDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                         wxWindow* parent, const wxString& caption,
                                         const wxWindowID id, const wxPoint& pos,
                                         const wxSize& size, const long style, EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode,
                         GraphDlgIncludeAll)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    std::vector<DataGuideParameter> InsertLinePlotDlg::GetParameters()
        {
        // the order here is the order of the lists in the variable selection dialog
        return {
            { .m_name = /* TRANSLATORS: y-axis variable for line plot. */ _(L"Values"),
              .m_types = { Data::Dataset::ColumnImportType::Numeric },
              .m_requirement = DataGuideRequirement::Required,
              .m_description = _(L"The measurement being plotted on the Y axis, such as an "
                                 "average grade."),
              .m_values = {},
              .m_multiple = false,
              .m_condition = wxString{},
              .m_group = wxString{} },
            { .m_name = /* TRANSLATORS: x-axis variable for line plot. */ _(L"Series"),
              .m_types = { Data::Dataset::ColumnImportType::Numeric,
                           Data::Dataset::ColumnImportType::String,
                           Data::Dataset::ColumnImportType::Discrete,
                           Data::Dataset::ColumnImportType::DichotomousString,
                           Data::Dataset::ColumnImportType::DichotomousDiscrete },
              .m_requirement = DataGuideRequirement::Required,
              .m_description = _(L"The position of each point along the X axis, such as a week "
                                 "or a year."),
              .m_values = {},
              .m_multiple = false,
              .m_condition = wxString{},
              .m_group = wxString{} },
            { .m_name = _(L"Grouping"),
              .m_types = { Data::Dataset::ColumnImportType::String,
                           Data::Dataset::ColumnImportType::Discrete,
                           Data::Dataset::ColumnImportType::DichotomousString,
                           Data::Dataset::ColumnImportType::DichotomousDiscrete },
              .m_requirement = DataGuideRequirement::Optional,
              .m_description = _(L"Draws a separate line for each unique value, such as one line "
                                 "per gender. Without it, a single line is drawn."),
              .m_values = {},
              .m_multiple = false,
              .m_condition = wxString{},
              .m_group = wxString{} }
        };
        }

    //-------------------------------------------
    DataGuide InsertLinePlotDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Line Plot");
        guide.m_iconName = L"images/lineplot.svg";
        guide.m_summary =
            _(L"Shows how a measurement changes across a series, such as weeks or years, "
              "with an optional separate line for each group.");
        guide.m_requirements =
            _(L"One row per point, holding the measurement, its position along the X axis, and "
              "optionally the group that the point belongs to.");
        guide.m_shape = DataGuideShape::Long;
        guide.m_parameters = GetParameters();

        DataGuideExample grades;
        grades.m_title = _(L"Example: average spelling grades by week for two groups");
        grades.m_columns = { { _(L"Week"), _(L"Series") },
                             { _(L"Average"), _(L"Values") },
                             { _(L"Gender"), _(L"Grouping") } };
        grades.m_rows = { { L"1", L"82", _(L"Female") }, { L"2", L"85", _(L"Female") },
                          { L"3", L"88", _(L"Female") }, { L"1", L"79", _(L"Male") },
                          { L"2", L"81", _(L"Male") },   { L"3", L"84", _(L"Male") } };
        guide.m_examples = { std::move(grades) };

        guide.m_missingData = {
            _(L"A missing value in either the Values or Series column breaks the line at that "
              "row. The next valid row starts the line again."),
            _(L"A missing value in the grouping column is shown with an empty legend label.")
        };
        guide.m_notes = { _(L"Row order matters. Each line is drawn through its points in the "
                            "order that they appear in the dataset, not sorted by the X axis.") };
        return guide;
        }

    //-------------------------------------------
    void InsertLinePlotDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Line Plot"), ID_OPTIONS_SECTION, true);

        // dataset selector
        auto* datasetSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        datasetSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Dataset:")),
                          wxSizerFlags{}.CenterVertical());
        auto* datasetChoice = CreateDatasetChoice(optionsPage);
        datasetSizer->Add(datasetChoice);

        optionsSizer->Add(datasetSizer, wxSizerFlags{}.Border());

        // variables button
        auto* varsBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Variables"));
        auto* varButton =
            new wxButton(varsBox->GetStaticBox(), ID_SELECT_VARS_BUTTON, _(L"Select..."));
        varsBox->Add(varButton, wxSizerFlags{}.Border(wxLEFT));

        // variable label grid
        auto* varGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(12), FromDIP(2) });

        auto* yLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Values:"));
        yLabel->SetFont(yLabel->GetFont().Bold());
        varGrid->Add(yLabel, wxSizerFlags{}.CenterVertical());
        m_yVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_yVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_yVarLabel, wxSizerFlags{}.CenterVertical());

        auto* xLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Series:"));
        xLabel->SetFont(xLabel->GetFont().Bold());
        varGrid->Add(xLabel, wxSizerFlags{}.CenterVertical());
        m_xVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_xVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_xVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Grouping:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // line options
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Auto spline"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator{ &m_autoSpline }),
                          wxSizerFlags{}.Border());

        // showcasing
        auto* ghostBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Showcasing"));

        auto* ghostOpacitySizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        ghostOpacitySizer->Add(
            new wxStaticText(ghostBox->GetStaticBox(), wxID_ANY, _(L"Ghost opacity:")),
            wxSizerFlags{}.CenterVertical());
        auto* opacitySpin = new wxSpinCtrl(ghostBox->GetStaticBox(), wxID_ANY);
        opacitySpin->SetRange(0, 255);
        opacitySpin->SetValidator(wxGenericValidator{ &m_ghostOpacity });
        ghostOpacitySizer->Add(opacitySpin);
        ghostBox->Add(ghostOpacitySizer, wxSizerFlags{}.Border());

        m_showcaseListBox = new wxEditableListBox(
            ghostBox->GetStaticBox(), wxID_ANY, _(L"Showcase lines:"), wxDefaultPosition,
            wxSize{ FromDIP(300), FromDIP(120) },
            wxEL_ALLOW_NEW | wxEL_ALLOW_DELETE | wxEL_ALLOW_EDIT | wxEL_NO_REORDER);
        ghostBox->Add(m_showcaseListBox, wxSizerFlags{ 1 }.Expand().Border());
        optionsSizer->Add(ghostBox, wxSizerFlags{ 1 }.Expand().Border());

        // override New button for showcase lines
        m_showcaseListBox->GetNewButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                const auto dataset = GetSelectedDataset();
                if (dataset == nullptr || m_groupVariable.empty())
                    {
                    wxMessageBox(
                        _(L"Select a grouping variable first to populate available lines."),
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

                wxSingleChoiceDialog dlg(this, _(L"Select line to showcase:"), _(L"Showcase Line"),
                                         groupChoices);
                if (dlg.ShowModal() == wxID_OK)
                    {
                    const auto val = dlg.GetStringSelection();
                    if (std::find(m_showcaseLines.begin(), m_showcaseLines.end(), val) ==
                        m_showcaseLines.end())
                        {
                        m_showcaseLines.push_back(val);
                        wxArrayString strings;
                        for (const auto& showLine : m_showcaseLines)
                            {
                            strings.Add(showLine);
                            }
                        m_showcaseListBox->SetStrings(strings);
                        }
                    }
            });

        // override Edit button
        m_showcaseListBox->GetEditButton()->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
                                                 { OnEditShowcasedLine(); });
        m_showcaseListBox->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                                [this](wxListEvent&) { OnEditShowcasedLine(); });

        // override Delete button
        m_showcaseListBox->GetDelButton()->Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event)
            {
                const long sel = m_showcaseListBox->GetListCtrl()->GetNextItem(
                    -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
                if (sel < 0 || std::cmp_greater_equal(sel, m_showcaseLines.size()))
                    {
                    return;
                    }
                m_showcaseLines.erase(m_showcaseLines.begin() + sel);
                wxArrayString strings;
                for (const auto& s : m_showcaseLines)
                    {
                    strings.Add(s);
                    }
                m_showcaseListBox->SetStrings(strings);
            });

        // bind events
        datasetChoice->Bind(wxEVT_CHOICE,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateLegendOptionsPage();
        CreateAnnotationsPage();
        CreateAxisOptionsPage();
        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertLinePlotDlg::OnEditShowcasedLine()
        {
        const auto dataset = GetSelectedDataset();
        const long sel = m_showcaseListBox->GetListCtrl()->GetNextItem(-1, wxLIST_NEXT_ALL,
                                                                       wxLIST_STATE_SELECTED);
        if (sel < 0 || std::cmp_greater_equal(sel, m_showcaseLines.size()) || dataset == nullptr ||
            m_groupVariable.empty())
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

        wxSingleChoiceDialog dlg(this, _(L"Select line to showcase:"), _(L"Showcase Line"),
                                 groupChoices);
        dlg.SetSelection(sel);
        if (dlg.ShowModal() == wxID_OK)
            {
            m_showcaseLines[sel] = dlg.GetStringSelection();
            wxArrayString strings;
            for (const auto& showLine : m_showcaseLines)
                {
                strings.Add(showLine);
                }
            m_showcaseListBox->SetStrings(strings);
            }
        }

    //-------------------------------------------
    void InsertLinePlotDlg::OnDatasetChanged()
        {
        m_xVariable.clear();
        m_yVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        ResetAxisOptions();
        }

    //-------------------------------------------
    void InsertLinePlotDlg::OnSelectVariables()
        {
        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset first."), _(L"No Dataset"),
                         wxOK | wxICON_INFORMATION, this);
            return;
            }

        // prefer the stored column preview info (preserves original file order)
        // over rebuilding it from the dataset's internal column grouping
        Data::Dataset::ColumnPreviewInfo columnInfo;
        if (GetReportBuilder() != nullptr)
            {
            const auto& importOpts = GetReportBuilder()->GetDatasetImportOptions();
            const auto it = importOpts.find(GetSelectedDatasetName());
            if (it != importOpts.cend())
                {
                columnInfo = it->second.m_columnPreviewInfo;
                }
            }
        if (columnInfo.empty())
            {
            columnInfo = BuildColumnPreviewInfo(*dataset);
            }

        const auto parameters = GetParameters();
        VariableSelectDlg dlg(
            this, columnInfo,
            { MakeVariableList(parameters[0], m_yVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_yVariable }),
              MakeVariableList(parameters[1], m_xVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_xVariable }),
              MakeVariableList(parameters[2], m_groupVariable.empty() ?
                                                  std::vector<wxString>{} :
                                                  std::vector<wxString>{ m_groupVariable }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto yVars = dlg.GetSelectedVariables(0);
        m_yVariable = yVars.empty() ? wxString{} : yVars.front();

        const auto xVars = dlg.GetSelectedVariables(1);
        m_xVariable = xVars.empty() ? wxString{} : xVars.front();

        const auto groupVars = dlg.GetSelectedVariables(2);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        ResetAxisOptions();
        }

    //-------------------------------------------
    void InsertLinePlotDlg::UpdateVariableLabels()
        {
        m_yVarLabel->SetLabel(m_yVariable);
        m_xVarLabel->SetLabel(m_xVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        // clear showcase if group changes
        m_showcaseLines.clear();
        if (m_showcaseListBox != nullptr)
            {
            m_showcaseListBox->SetStrings(wxArrayString{});
            }

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    bool InsertLinePlotDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_yVariable.empty() || m_xVariable.empty())
            {
            wxMessageBox(_(L"Please select the Y and X variables."), _(L"Variable Not Specified"),
                         wxOK | wxICON_WARNING, this);
            OnSelectVariables();
            return false;
            }

        if (!ValidateColorScheme())
            {
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertLinePlotDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* linePlot = dynamic_cast<const Graphs::LinePlot*>(&graph);
        if (linePlot == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        SelectDataset(linePlot->GetPropertyTemplate(L"dataset"));

        // load the actual column names from the graph
        m_xVariable = linePlot->GetXColumnName();
        m_yVariable = linePlot->GetYColumnName();
        m_groupVariable = linePlot->GetGroupColumnName().value_or(wxString{});
        UpdateVariableLabels();

        // line-specific options
        m_autoSpline = linePlot->IsAutoSplining();
        m_ghostOpacity = linePlot->GetGhostOpacity();
        m_showcaseLines = linePlot->GetShowcasedLines();
        if (m_showcaseListBox != nullptr)
            {
            wxArrayString strings;
            for (const auto& showLine : m_showcaseLines)
                {
                strings.Add(showLine);
                }
            m_showcaseListBox->SetStrings(strings);
            }

        TransferDataToWindow();
        }

    //-------------------------------------------
    std::shared_ptr<Graphs::LinePlot>
    InsertLinePlotDlg::BuildLinePlot(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::LinePlot>(GetCanvas());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);
        plot->AutoSpline(GetAutoSpline());
        plot->SetShapeScheme(GetShapeScheme());

        const std::optional<wxString> groupCol =
            GetGroupVariable().empty() ? std::nullopt : std::optional<wxString>(GetGroupVariable());
        plot->SetData(GetSelectedDataset(), GetYVariable(), GetXVariable(), groupCol);
        plot->SetGhostOpacity(GetGhostOpacity());
        if (!GetShowcaseLines().empty())
            {
            plot->ShowcaseLines(GetShowcaseLines());
            }

        ApplyAxisOverrides(*plot);

        if (oldGraph != nullptr)
            {
            const auto* oldLine = dynamic_cast<const Graphs::LinePlot*>(oldGraph);

            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"variables.x", GetXVariable(),
                                               oldLine != nullptr ? oldLine->GetXColumnName() :
                                                                    wxString{});
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"variables.y", GetYVariable(),
                                               oldLine != nullptr ? oldLine->GetYColumnName() :
                                                                    wxString{});
            const auto oldGroupName = (oldLine != nullptr) ?
                                          oldLine->GetGroupColumnName().value_or(wxString{}) :
                                          wxString{};
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"variables.group",
                                               GetGroupVariable(), oldGroupName);
            }
        else
            {
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            plot->SetPropertyTemplate(L"variables.y", GetYVariable());
            plot->SetPropertyTemplate(L"variables.x", GetXVariable());
            if (!GetGroupVariable().empty())
                {
                plot->SetPropertyTemplate(L"variables.group", GetGroupVariable());
                }
            }

        // showcase bars/lines/bins
        for (size_t i = 0; i < GetShowcaseLines().size(); ++i)
            {
            plot->SetPropertyTemplate(wxString::Format(_DT(L"showcase-lines[%zu]"), i),
                                      GetShowcaseLines()[i]);
            }

        return plot;
        }
    } // namespace Wisteria::UI
