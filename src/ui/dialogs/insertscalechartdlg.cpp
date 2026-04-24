///////////////////////////////////////////////////////////////////////////////
// Name:        insertscalechartdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertscalechartdlg.h"
#include "../../graphs/scalechart.h"
#include "variableselectdlg.h"
#include <wx/clrpicker.h>
#include <wx/numdlg.h>
#include <wx/tokenzr.h>
#include <wx/valgen.h>
#include <wx/valtext.h>

namespace Wisteria::UI
    {

    // block edit sub-dialog
    //----------------------
    class EditBlockDlg : public wxDialog
        {
      public:
        EditBlockDlg(wxWindow* parent, const InsertScaleChartDlg::BlockInfo& block)
            : wxDialog(parent, wxID_ANY, _(L"Edit Block"), wxDefaultPosition, wxDefaultSize,
                       wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
              m_label(block.m_label), m_length(block.m_length), m_color(block.m_color)
            {
            auto* mainSizer = new wxBoxSizer(wxVERTICAL);

            auto* grid = new wxFlexGridSizer(2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2,
                                                        wxSizerFlags::GetDefaultBorder() });
            grid->AddGrowableCol(1, 1);

            grid->Add(new wxStaticText(this, wxID_ANY, _(L"Label:")),
                      wxSizerFlags{}.CenterVertical());
            grid->Add(new wxTextCtrl(this, wxID_ANY, wxString{}, wxDefaultPosition,
                                     wxSize{ FromDIP(200), -1 }, 0,
                                     wxTextValidator(wxFILTER_NONE, &m_label)),
                      wxSizerFlags{}.Expand());

            grid->Add(new wxStaticText(this, wxID_ANY, _(L"Length:")),
                      wxSizerFlags{}.CenterVertical());
            m_lengthCtrl =
                new wxSpinCtrlDouble(this, wxID_ANY, wxString{}, wxDefaultPosition, wxDefaultSize,
                                     wxSP_ARROW_KEYS, 0.1, 10000, m_length, 0.5);
            grid->Add(m_lengthCtrl);

            grid->Add(new wxStaticText(this, wxID_ANY, _(L"Color:")),
                      wxSizerFlags{}.CenterVertical());
            m_colorPicker = new wxColourPickerCtrl(this, wxID_ANY, m_color);
            grid->Add(m_colorPicker);

            mainSizer->Add(grid, wxSizerFlags{ 1 }.Expand().Border());

            auto* btnSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
            mainSizer->Add(btnSizer, wxSizerFlags{}.Expand().Border());

            SetSizerAndFit(mainSizer);
            Centre();
            }

        [[nodiscard]]
        InsertScaleChartDlg::BlockInfo GetBlock() const
            {
            return { m_label, m_lengthCtrl->GetValue(), m_colorPicker->GetColour() };
            }

      private:
        bool TransferDataFromWindow() override
            {
            if (!wxDialog::TransferDataFromWindow())
                {
                return false;
                }
            return true;
            }

        wxString m_label;
        double m_length{ 10 };
        wxColour m_color{ *wxGREEN };
        wxSpinCtrlDouble* m_lengthCtrl{ nullptr };
        wxColourPickerCtrl* m_colorPicker{ nullptr };
        };

    //-------------------------------------------
    InsertScaleChartDlg::InsertScaleChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                             wxWindow* parent, const wxString& caption,
                                             const wxWindowID id, const wxPoint& pos,
                                             const wxSize& size, const long style,
                                             EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertScaleChartDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Scale Chart"), ID_OPTIONS_SECTION, true);

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
                m_datasetNames.emplace_back(name);
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

        auto* scoreLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Score:"));
        scoreLabel->SetFont(scoreLabel->GetFont().Bold());
        varGrid->Add(scoreLabel, wxSizerFlags{}.CenterVertical());
        m_scoreVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_scoreVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_scoreVarLabel, wxSizerFlags{}.CenterVertical());

        auto* groupLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, _(L"Grouping:"));
        groupLabel->SetFont(groupLabel->GetFont().Bold());
        varGrid->Add(groupLabel, wxSizerFlags{}.CenterVertical());
        m_groupVarLabel = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
        m_groupVarLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        varGrid->Add(m_groupVarLabel, wxSizerFlags{}.CenterVertical());

        varsBox->Add(varGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // main scale values and headers
        auto* scaleOptGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        scaleOptGrid->AddGrowableCol(1, 1);

        scaleOptGrid->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Scale values:")),
                          wxSizerFlags{}.CenterVertical());
        scaleOptGrid->Add(new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                         wxDefaultSize, 0,
                                         wxTextValidator(wxFILTER_NONE, &m_mainScaleValues)),
                          wxSizerFlags{}.Expand());

        scaleOptGrid->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Precision:")),
                          wxSizerFlags{}.CenterVertical());
        auto* scalePrecisionSpin =
            new wxSpinCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition, wxDefaultSize,
                           wxSP_ARROW_KEYS, 0, 10, 0);
        scalePrecisionSpin->SetValidator(wxGenericValidator(&m_mainScalePrecision));
        scaleOptGrid->Add(scalePrecisionSpin);

        scaleOptGrid->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Scale header:")),
                          wxSizerFlags{}.CenterVertical());
        scaleOptGrid->Add(new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                         wxDefaultSize, 0,
                                         wxTextValidator(wxFILTER_NONE, &m_mainScaleHeader)),
                          wxSizerFlags{}.Expand());

        scaleOptGrid->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Data header:")),
                          wxSizerFlags{}.CenterVertical());
        scaleOptGrid->Add(new wxTextCtrl(optionsPage, wxID_ANY, wxString{}, wxDefaultPosition,
                                         wxDefaultSize, 0,
                                         wxTextValidator(wxFILTER_NONE, &m_dataColumnHeader)),
                          wxSizerFlags{}.Expand());

        optionsSizer->Add(scaleOptGrid, wxSizerFlags{}.Expand().Border());

        // showcase score checkbox
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Showcase score"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showcaseScore)),
                          wxSizerFlags{}.Border());

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

        // scales page
        CreateScalesPage();

        // prefill with example scales for US academic grade scales
        // (LoadFromGraph will overwrite these for edit mode)
        const wxColour pastelRed{ 255, 129, 123, 150 };
        const wxColour corn{ 251, 236, 93, 150 };
        const wxColour evergreenFog{ 183, 200, 175, 150 };
        const wxColour fernGreen{ 79, 121, 66, 150 };
        const wxColour emerald{ 80, 200, 120, 150 };
        m_scales = { { _(L"Grades"),
                       std::nullopt,
                       { { _(L"F (fail)"), 59, pastelRed },
                         { L"D", 10, corn },
                         { L"C", 10, evergreenFog },
                         { L"B", 10, fernGreen },
                         { L"A", 10, emerald } } },
                     { _(L"Grades"),
                       std::nullopt,
                       { { _(L"F (fail)"), 59, pastelRed },
                         { L"D-", 3, corn },
                         { L"D", 4, corn },
                         { L"D+", 3, corn },
                         { L"C-", 3, evergreenFog },
                         { L"C", 4, evergreenFog },
                         { L"C+", 3, evergreenFog },
                         { L"B-", 3, fernGreen },
                         { L"B", 4, fernGreen },
                         { L"B+", 3, fernGreen },
                         { L"A-", 3, emerald },
                         { L"A", 4, emerald },
                         { L"A+", 3, emerald } } } };
        m_mainScaleValues = L"10, 20, 30, 40, 50, 60, 70, 80, 90";
        m_dataColumnHeader = _(L"Test Scores");
        RefreshScalesList();
        }

    //-------------------------------------------
    void InsertScaleChartDlg::CreateScalesPage()
        {
        auto* scalesPage = new wxPanel(GetSideBarBook());
        auto* pageMainSizer = new wxBoxSizer(wxVERTICAL);
        scalesPage->SetSizer(pageMainSizer);
        GetSideBarBook()->AddPage(scalesPage, _(L"Scales"), ID_SCALES_SECTION);

        // scales list + buttons
        auto* scalesBox = new wxStaticBoxSizer(wxVERTICAL, scalesPage, _(L"Scales"));

        m_scalesList = new wxListView(scalesBox->GetStaticBox(), wxID_ANY, wxDefaultPosition,
                                      wxSize{ -1, FromDIP(80) }, wxLC_REPORT | wxLC_SINGLE_SEL);
        m_scalesList->InsertColumn(0, _(L"Header"), wxLIST_FORMAT_LEFT, FromDIP(200));
        m_scalesList->InsertColumn(1, _(L"Blocks"), wxLIST_FORMAT_LEFT, FromDIP(60));
        scalesBox->Add(m_scalesList, wxSizerFlags{ 1 }.Expand().Border());

        auto* scaleBtnSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* addScaleBtn = new wxButton(scalesBox->GetStaticBox(), wxID_ANY, _(L"Add"));
        auto* removeScaleBtn = new wxButton(scalesBox->GetStaticBox(), wxID_ANY, _(L"Remove"));
        scaleBtnSizer->Add(addScaleBtn,
                           wxSizerFlags{}.Border(wxRIGHT, wxSizerFlags::GetDefaultBorder()));
        scaleBtnSizer->Add(removeScaleBtn);
        scalesBox->Add(scaleBtnSizer, wxSizerFlags{}.Border(wxLEFT));

        // scale header edit
        auto* headerSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        headerSizer->AddGrowableCol(1, 1);
        headerSizer->Add(new wxStaticText(scalesBox->GetStaticBox(), wxID_ANY, _(L"Header:")),
                         wxSizerFlags{}.CenterVertical());
        m_scaleHeaderCtrl = new wxTextCtrl(scalesBox->GetStaticBox(), wxID_ANY);
        headerSizer->Add(m_scaleHeaderCtrl, wxSizerFlags{}.Expand());
        scalesBox->Add(headerSizer, wxSizerFlags{}.Expand().Border());

        pageMainSizer->Add(scalesBox, wxSizerFlags{ 1 }.Expand().Border());

        // blocks list + buttons
        auto* blocksBox = new wxStaticBoxSizer(wxVERTICAL, scalesPage, _(L"Blocks"));

        m_blocksList = new wxListView(blocksBox->GetStaticBox(), wxID_ANY, wxDefaultPosition,
                                      wxSize{ -1, FromDIP(150) }, wxLC_REPORT | wxLC_SINGLE_SEL);
        m_blocksList->InsertColumn(0, _(L"Label"), wxLIST_FORMAT_LEFT, FromDIP(120));
        m_blocksList->InsertColumn(1, _(L"Length"), wxLIST_FORMAT_RIGHT, FromDIP(60));
        m_blocksList->InsertColumn(2, _(L"Color"), wxLIST_FORMAT_LEFT, FromDIP(80));
        blocksBox->Add(m_blocksList, wxSizerFlags{ 1 }.Expand().Border());

        auto* blockBtnSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* addBlockBtn = new wxButton(blocksBox->GetStaticBox(), wxID_ANY, _(L"Add"));
        auto* editBlockBtn = new wxButton(blocksBox->GetStaticBox(), wxID_ANY, _(L"Edit"));
        auto* removeBlockBtn = new wxButton(blocksBox->GetStaticBox(), wxID_ANY, _(L"Remove"));
        auto* moveUpBtn = new wxButton(blocksBox->GetStaticBox(), wxID_ANY, wxString(L"\u25B2"));
        auto* moveDownBtn = new wxButton(blocksBox->GetStaticBox(), wxID_ANY, wxString(L"\u25BC"));
        blockBtnSizer->Add(addBlockBtn,
                           wxSizerFlags{}.Border(wxRIGHT, wxSizerFlags::GetDefaultBorder()));
        blockBtnSizer->Add(editBlockBtn,
                           wxSizerFlags{}.Border(wxRIGHT, wxSizerFlags::GetDefaultBorder()));
        blockBtnSizer->Add(removeBlockBtn,
                           wxSizerFlags{}.Border(wxRIGHT, wxSizerFlags::GetDefaultBorder()));
        blockBtnSizer->Add(moveUpBtn,
                           wxSizerFlags{}.Border(wxRIGHT, wxSizerFlags::GetDefaultBorder()));
        blockBtnSizer->Add(moveDownBtn);
        blocksBox->Add(blockBtnSizer, wxSizerFlags{}.Border(wxLEFT));

        pageMainSizer->Add(blocksBox, wxSizerFlags{ 1 }.Expand().Border());

        // bind events
        m_scalesList->Bind(wxEVT_LIST_ITEM_SELECTED,
                           [this]([[maybe_unused]] wxListEvent&) { OnScaleSelected(); });

        m_scaleHeaderCtrl->Bind(wxEVT_TEXT,
                                [this]([[maybe_unused]] wxCommandEvent&)
                                {
                                    const long sel = GetSelectedScaleIndex();
                                    if (sel != wxNOT_FOUND)
                                        {
                                        m_scales[sel].m_header = m_scaleHeaderCtrl->GetValue();
                                        m_scalesList->SetItemText(sel, m_scales[sel].m_header);
                                        }
                                });

        addScaleBtn->Bind(wxEVT_BUTTON, [this]([[maybe_unused]] wxCommandEvent&) { OnAddScale(); });
        removeScaleBtn->Bind(wxEVT_BUTTON,
                             [this]([[maybe_unused]] wxCommandEvent&) { OnRemoveScale(); });
        addBlockBtn->Bind(wxEVT_BUTTON, [this]([[maybe_unused]] wxCommandEvent&) { OnAddBlock(); });
        editBlockBtn->Bind(wxEVT_BUTTON,
                           [this]([[maybe_unused]] wxCommandEvent&) { OnEditBlock(); });
        removeBlockBtn->Bind(wxEVT_BUTTON,
                             [this]([[maybe_unused]] wxCommandEvent&) { OnRemoveBlock(); });
        moveUpBtn->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnMoveBlockUp(); });
        moveDownBtn->Bind(wxEVT_BUTTON,
                          [this]([[maybe_unused]] wxCommandEvent&) { OnMoveBlockDown(); });

        m_blocksList->Bind(wxEVT_LIST_ITEM_ACTIVATED,
                           [this]([[maybe_unused]] wxListEvent&) { OnEditBlock(); });
        }

    //-------------------------------------------
    long InsertScaleChartDlg::GetSelectedScaleIndex() const
        {
        return m_scalesList->GetFirstSelected();
        }

    //-------------------------------------------
    long InsertScaleChartDlg::GetSelectedBlockIndex() const
        {
        return m_blocksList->GetFirstSelected();
        }

    //-------------------------------------------
    void InsertScaleChartDlg::RefreshScalesList()
        {
        const wxWindowUpdateLocker noUpdates(m_scalesList);
        m_scalesList->DeleteAllItems();
        for (size_t i = 0; i < m_scales.size(); ++i)
            {
            const long idx = m_scalesList->InsertItem(static_cast<long>(i), m_scales[i].m_header);
            m_scalesList->SetItem(idx, 1, std::to_wstring(m_scales[i].m_blocks.size()));
            }
        }

    //-------------------------------------------
    void InsertScaleChartDlg::RefreshBlocksList()
        {
        const wxWindowUpdateLocker noUpdates(m_blocksList);
        m_blocksList->DeleteAllItems();
        const long scaleSel = GetSelectedScaleIndex();
        if (scaleSel == wxNOT_FOUND)
            {
            return;
            }

        const auto& blocks = m_scales[scaleSel].m_blocks;
        for (size_t i = 0; i < blocks.size(); ++i)
            {
            const long idx = m_blocksList->InsertItem(static_cast<long>(i), blocks[i].m_label);
            m_blocksList->SetItem(idx, 1, wxString::FromDouble(blocks[i].m_length, 1));
            m_blocksList->SetItem(idx, 2, blocks[i].m_color.GetAsString(wxC2S_HTML_SYNTAX));
            m_blocksList->SetItemBackgroundColour(idx, blocks[i].m_color);
            m_blocksList->SetItemTextColour(
                idx, Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(blocks[i].m_color));
            }
        }

    //-------------------------------------------
    void InsertScaleChartDlg::OnScaleSelected()
        {
        const long sel = GetSelectedScaleIndex();
        if (sel != wxNOT_FOUND)
            {
            m_scaleHeaderCtrl->ChangeValue(m_scales[sel].m_header);
            }
        else
            {
            m_scaleHeaderCtrl->ChangeValue(wxString{});
            }
        RefreshBlocksList();
        }

    //-------------------------------------------
    void InsertScaleChartDlg::OnAddScale()
        {
        m_scales.emplace_back(ScaleInfo{ _(L"New Scale"), std::nullopt, {} });
        RefreshScalesList();
        // select the new scale
        const long newIdx = static_cast<long>(m_scales.size()) - 1;
        m_scalesList->Select(newIdx);
        m_scalesList->EnsureVisible(newIdx);
        OnScaleSelected();
        }

    //-------------------------------------------
    void InsertScaleChartDlg::OnRemoveScale()
        {
        const long sel = GetSelectedScaleIndex();
        if (sel == wxNOT_FOUND)
            {
            return;
            }
        m_scales.erase(m_scales.begin() + sel);
        RefreshScalesList();
        if (!m_scales.empty())
            {
            const long newSel = std::min(sel, static_cast<long>(m_scales.size()) - 1);
            m_scalesList->Select(newSel);
            }
        OnScaleSelected();
        }

    //-------------------------------------------
    void InsertScaleChartDlg::OnAddBlock()
        {
        const long scaleSel = GetSelectedScaleIndex();
        if (scaleSel == wxNOT_FOUND)
            {
            wxMessageBox(_(L"Please select a scale first."), _(L"No Scale Selected"),
                         wxOK | wxICON_INFORMATION, this);
            return;
            }

        EditBlockDlg dlg(this, BlockInfo{ wxString{}, 10, *wxGREEN });
        if (dlg.ShowModal() == wxID_OK)
            {
            m_scales[scaleSel].m_blocks.emplace_back(dlg.GetBlock());
            RefreshBlocksList();
            RefreshScalesList();
            }
        }

    //-------------------------------------------
    void InsertScaleChartDlg::OnEditBlock()
        {
        const long scaleSel = GetSelectedScaleIndex();
        const long blockSel = GetSelectedBlockIndex();
        if (scaleSel == wxNOT_FOUND || blockSel == wxNOT_FOUND)
            {
            return;
            }

        EditBlockDlg dlg(this, m_scales[scaleSel].m_blocks[blockSel]);
        if (dlg.ShowModal() == wxID_OK)
            {
            m_scales[scaleSel].m_blocks[blockSel] = dlg.GetBlock();
            RefreshBlocksList();
            m_blocksList->Select(blockSel);
            }
        }

    //-------------------------------------------
    void InsertScaleChartDlg::OnRemoveBlock()
        {
        const long scaleSel = GetSelectedScaleIndex();
        const long blockSel = GetSelectedBlockIndex();
        if (scaleSel == wxNOT_FOUND || blockSel == wxNOT_FOUND)
            {
            return;
            }

        auto& blocks = m_scales[scaleSel].m_blocks;
        blocks.erase(blocks.begin() + blockSel);
        RefreshBlocksList();
        RefreshScalesList();
        if (!blocks.empty())
            {
            const long newSel = std::min(blockSel, static_cast<long>(blocks.size()) - 1);
            m_blocksList->Select(newSel);
            }
        }

    //-------------------------------------------
    void InsertScaleChartDlg::OnMoveBlockUp()
        {
        const long scaleSel = GetSelectedScaleIndex();
        const long blockSel = GetSelectedBlockIndex();
        if (scaleSel == wxNOT_FOUND || blockSel <= 0)
            {
            return;
            }

        auto& blocks = m_scales[scaleSel].m_blocks;
        std::swap(blocks[blockSel], blocks[blockSel - 1]);
        RefreshBlocksList();
        m_blocksList->Select(blockSel - 1);
        }

    //-------------------------------------------
    void InsertScaleChartDlg::OnMoveBlockDown()
        {
        const long scaleSel = GetSelectedScaleIndex();
        const long blockSel = GetSelectedBlockIndex();
        if (scaleSel == wxNOT_FOUND || blockSel == wxNOT_FOUND)
            {
            return;
            }

        auto& blocks = m_scales[scaleSel].m_blocks;
        if (std::cmp_greater_equal(blockSel + 1, blocks.size()))
            {
            return;
            }
        std::swap(blocks[blockSel], blocks[blockSel + 1]);
        RefreshBlocksList();
        m_blocksList->Select(blockSel + 1);
        }

    //-------------------------------------------
    void InsertScaleChartDlg::OnDatasetChanged()
        {
        m_scoreVariable.clear();
        m_groupVariable.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertScaleChartDlg::OnSelectVariables()
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
                  .Label(_(L"Score"))
                  .SingleSelection(true)
                  .Required(true)
                  .DefaultVariables(m_scoreVariable.empty() ?
                                        std::vector<wxString>{} :
                                        std::vector<wxString>{ m_scoreVariable })
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

        const auto scoreVars = dlg.GetSelectedVariables(0);
        m_scoreVariable = scoreVars.empty() ? wxString{} : scoreVars.front();

        const auto groupVars = dlg.GetSelectedVariables(1);
        m_groupVariable = groupVars.empty() ? wxString{} : groupVars.front();

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertScaleChartDlg::UpdateVariableLabels()
        {
        m_scoreVarLabel->SetLabel(m_scoreVariable);
        m_groupVarLabel->SetLabel(m_groupVariable);

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertScaleChartDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertScaleChartDlg::GetSelectedDataset() const
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
    std::vector<double> InsertScaleChartDlg::GetMainScaleValues() const
        {
        std::vector<double> values;
        wxStringTokenizer tokenizer(m_mainScaleValues, L",; ");
        while (tokenizer.HasMoreTokens())
            {
            double val{ 0 };
            if (tokenizer.GetNextToken().Trim().Trim(false).ToCDouble(&val))
                {
                values.emplace_back(val);
                }
            }
        return values;
        }

    //-------------------------------------------
    bool InsertScaleChartDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        if (m_scoreVariable.empty())
            {
            wxMessageBox(_(L"Please select the score variable."), _(L"Variable Not Specified"),
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
    void InsertScaleChartDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* scaleChart = dynamic_cast<const Graphs::ScaleChart*>(&graph);
        if (scaleChart == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = scaleChart->GetPropertyTemplate(L"dataset");
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
        m_scoreVariable = scaleChart->GetScoresColumnName();
        m_groupVariable = scaleChart->GetGroupColumnName().value_or(wxString{});
        UpdateVariableLabels();

        // main scale values
        const auto& scaleValues = scaleChart->GetMainScaleValues();
        wxString valuesStr;
        for (size_t i = 0; i < scaleValues.size(); ++i)
            {
            if (i > 0)
                {
                valuesStr += L", ";
                }
            valuesStr += wxString::FromCDouble(scaleValues[i], scaleChart->GetMainScalePrecision());
            }
        m_mainScaleValues = valuesStr;
        m_mainScalePrecision = scaleChart->GetMainScalePrecision();

        // options
        m_showcaseScore = scaleChart->IsShowcasingScore();

        // scales from bars — skip the first two bars (main scale + scores)
        m_scales.clear();
        const auto& bars = scaleChart->GetBars();
        for (size_t barIdx = 2; barIdx < bars.size(); ++barIdx)
            {
            ScaleInfo scale;
            // try to get the header from the opposite bar axis custom label
            const auto& customLabels = scaleChart->GetOppositeBarAxis().GetCustomLabels();
            for (const auto& [pos, label] : customLabels)
                {
                if (compare_doubles(pos, bars[barIdx].GetAxisPosition()))
                    {
                    scale.m_header = label.GetText();
                    break;
                    }
                }
            scale.m_startPosition = bars[barIdx].GetCustomScalingAxisStartPosition();
            for (const auto& block : bars[barIdx].GetBlocks())
                {
                BlockInfo blockInfo;
                blockInfo.m_label = block.GetDecal().GetText();
                blockInfo.m_length = block.GetLength();
                blockInfo.m_color = block.GetBrush().GetColour();
                scale.m_blocks.emplace_back(blockInfo);
                }
            m_scales.emplace_back(scale);
            }
        RefreshScalesList();
        if (!m_scales.empty())
            {
            m_scalesList->Select(0);
            OnScaleSelected();
            }

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
