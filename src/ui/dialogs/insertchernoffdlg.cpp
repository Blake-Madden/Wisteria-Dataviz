///////////////////////////////////////////////////////////////////////////////
// Name:        insertchernoffdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertchernoffdlg.h"
#include "variableselectdlg.h"

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertChernoffDlg::InsertChernoffDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                         wxWindow* parent, const wxWindowID id, const wxPoint& pos,
                                         const wxSize& size, const long style)
        : InsertGraphDlg(canvas, reportBuilder, parent, _(L"Insert Chernoff Faces Plot"), id, pos,
                         size, style)
        {
        CreateControls();
        FinalizeControls();

        const auto currentSize = GetSize();
        SetSize(currentSize.GetWidth() * 2, currentSize.GetHeight());
        SetMinSize(wxSize{ currentSize.GetWidth() * 2, currentSize.GetHeight() });

        Centre();
        }

    //-------------------------------------------
    void InsertChernoffDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();
        CreateGraphOptionsPage();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Chernoff Faces Options"), ID_OPTIONS_SECTION,
                                  true);

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
        auto* varButton = new wxButton(optionsPage, ID_SELECT_VARS_BUTTON, _(L"Variables..."));
        optionsSizer->Add(varButton, wxSizerFlags{}.Border(wxLEFT));

        // feature-to-variable label grid
        const wxColour varLabelColor{ 0, 102, 204 };

        using FID = Graphs::ChernoffFacesPlot::FeatureId;
        constexpr FID allFeatures[] = { FID::FaceWidth,   FID::FaceHeight,   FID::EyeSize,
                                        FID::EyePosition, FID::EyebrowSlant, FID::PupilDirection,
                                        FID::NoseSize,    FID::MouthWidth,   FID::SmileFrown,
                                        FID::FaceColor,   FID::EarSize };

        auto* featureGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(12), FromDIP(2) });

        for (size_t i = 0; i < FEATURE_COUNT; ++i)
            {
            auto* featureLabel = new wxStaticText(
                optionsPage, wxID_ANY,
                Graphs::ChernoffFacesPlot::GetFeatureDisplayName(allFeatures[i]) + L":");
            featureLabel->SetFont(featureLabel->GetFont().Bold());
            featureGrid->Add(featureLabel, wxSizerFlags{}.CenterVertical());

            m_featureVarLabels[i] = new wxStaticText(optionsPage, wxID_ANY, wxString{});
            m_featureVarLabels[i]->SetForegroundColour(varLabelColor);
            featureGrid->Add(m_featureVarLabels[i], wxSizerFlags{}.CenterVertical());
            }

        optionsSizer->Add(featureGrid, wxSizerFlags{}.Border());

        // appearance options
        auto* appearanceSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

        // gender
        appearanceSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Gender:")),
                             wxSizerFlags{}.CenterVertical());
        m_genderChoice = new wxChoice(optionsPage, wxID_ANY);
        m_genderChoice->Append(_(L"Female"));
        m_genderChoice->Append(_(L"Male"));
        m_genderChoice->SetSelection(0);
        appearanceSizer->Add(m_genderChoice);

        // hair style
        appearanceSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Hair style:")),
                             wxSizerFlags{}.CenterVertical());
        m_hairStyleChoice = new wxChoice(optionsPage, wxID_ANY);
        m_hairStyleChoice->Append(_(L"Bald"));
        m_hairStyleChoice->Append(_(L"Bob"));
        m_hairStyleChoice->Append(_(L"Pixie"));
        m_hairStyleChoice->Append(_(L"Bun"));
        m_hairStyleChoice->Append(_(L"Long straight"));
        m_hairStyleChoice->Append(_(L"High top fade"));
        m_hairStyleChoice->SetSelection(1);
        appearanceSizer->Add(m_hairStyleChoice);

        // facial hair
        m_facialHairLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"Facial hair:"));
        appearanceSizer->Add(m_facialHairLabel, wxSizerFlags{}.CenterVertical());
        m_facialHairChoice = new wxChoice(optionsPage, wxID_ANY);
        m_facialHairChoice->Append(_(L"Clean shaven"));
        m_facialHairChoice->Append(_(L"Five o'clock shadow"));
        m_facialHairChoice->SetSelection(0);
        appearanceSizer->Add(m_facialHairChoice);

        // skin color range (lighter and darker side by side)
        appearanceSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Skin color:")),
                             wxSizerFlags{}.CenterVertical());
        auto* skinColorSizer = new wxBoxSizer(wxHORIZONTAL);
        m_skinColorLighterPicker =
            new wxColourPickerCtrl(optionsPage, wxID_ANY, wxColour{ 255, 239, 219 });
        skinColorSizer->Add(m_skinColorLighterPicker, wxSizerFlags{}.CenterVertical());
        skinColorSizer->Add(new wxStaticText(optionsPage, wxID_ANY, L"\x2013"),
                            wxSizerFlags{}.CenterVertical().Border(wxLEFT | wxRIGHT, FromDIP(4)));
        m_skinColorDarkerPicker =
            new wxColourPickerCtrl(optionsPage, wxID_ANY, wxColour{ 255, 224, 189 });
        skinColorSizer->Add(m_skinColorDarkerPicker, wxSizerFlags{}.CenterVertical());
        appearanceSizer->Add(skinColorSizer);

        // eye color
        appearanceSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Eye color:")),
                             wxSizerFlags{}.CenterVertical());
        m_eyeColorPicker = new wxColourPickerCtrl(optionsPage, wxID_ANY, wxColour{ 143, 188, 143 });
        appearanceSizer->Add(m_eyeColorPicker);

        // hair color
        appearanceSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Hair color:")),
                             wxSizerFlags{}.CenterVertical());
        m_hairColorPicker = new wxColourPickerCtrl(optionsPage, wxID_ANY, wxColour{ 183, 82, 46 });
        appearanceSizer->Add(m_hairColorPicker);

        optionsSizer->Add(appearanceSizer, wxSizerFlags{}.Border());

        // show labels
        m_showLabelsCheck =
            new wxCheckBox(optionsPage, wxID_ANY, _(L"Show labels (from ID column)"));
        m_showLabelsCheck->SetValue(true);
        optionsSizer->Add(m_showLabelsCheck, wxSizerFlags{}.Border(wxLEFT));

        // legend placement
        auto* legendSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        legendSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Legend:")),
                         wxSizerFlags{}.CenterVertical());
        legendSizer->Add(CreateLegendPlacementChoice(optionsPage, 1));
        optionsSizer->Add(legendSizer, wxSizerFlags{}.Border());

        // bind events
        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });
        m_genderChoice->Bind(wxEVT_CHOICE,
                             [this]([[maybe_unused]] wxCommandEvent&)
                             {
                                 m_facialHairLabel->Enable(m_genderChoice->GetSelection() == 1);
                                 m_facialHairChoice->Enable(m_genderChoice->GetSelection() == 1);
                                 Refresh();
                             });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });
        }

    //-------------------------------------------
    void InsertChernoffDlg::OnDatasetChanged()
        {
        m_featureVariables.clear();
        UpdateFeatureLabels();
        }

    //-------------------------------------------
    void InsertChernoffDlg::OnSelectVariables()
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
            const int sel = m_datasetChoice->GetSelection();
            if (sel != wxNOT_FOUND && static_cast<size_t>(sel) < m_datasetNames.size())
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
            { VLI{}.Label(_(L"Face Width")).SingleSelection(true),
              VLI{}.Label(_(L"Face Height")).SingleSelection(true).Required(false),
              VLI{}.Label(_(L"Eye Size")).SingleSelection(true).Required(false),
              VLI{}.Label(_(L"Eye Position")).SingleSelection(true).Required(false),
              VLI{}.Label(_(L"Eyebrow Slant")).SingleSelection(true).Required(false),
              VLI{}.Label(_(L"Pupil Position")).SingleSelection(true).Required(false),
              VLI{}.Label(_(L"Nose Size")).SingleSelection(true).Required(false),
              VLI{}.Label(_(L"Mouth Width")).SingleSelection(true).Required(false),
              VLI{}.Label(_(L"Mouth Curvature")).SingleSelection(true).Required(false),
              VLI{}.Label(_(L"Face Color")).SingleSelection(true).Required(false),
              VLI{}.Label(_(L"Ear Size")).SingleSelection(true).Required(false) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        // map feature IDs to selected variable names
        using FID = Graphs::ChernoffFacesPlot::FeatureId;
        constexpr FID featureOrder[] = { FID::FaceWidth,   FID::FaceHeight,   FID::EyeSize,
                                         FID::EyePosition, FID::EyebrowSlant, FID::PupilDirection,
                                         FID::NoseSize,    FID::MouthWidth,   FID::SmileFrown,
                                         FID::FaceColor,   FID::EarSize };

        m_featureVariables.clear();
        for (size_t i = 0; i < std::size(featureOrder); ++i)
            {
            const auto vars = dlg.GetSelectedVariables(i);
            if (!vars.empty())
                {
                m_featureVariables[featureOrder[i]] = vars.front();
                }
            }

        UpdateFeatureLabels();
        }

    //-------------------------------------------
    void InsertChernoffDlg::UpdateFeatureLabels()
        {
        using FID = Graphs::ChernoffFacesPlot::FeatureId;
        constexpr FID allFeatures[] = { FID::FaceWidth,   FID::FaceHeight,   FID::EyeSize,
                                        FID::EyePosition, FID::EyebrowSlant, FID::PupilDirection,
                                        FID::NoseSize,    FID::MouthWidth,   FID::SmileFrown,
                                        FID::FaceColor,   FID::EarSize };

        for (size_t i = 0; i < FEATURE_COUNT; ++i)
            {
            const auto it = m_featureVariables.find(allFeatures[i]);
            m_featureVarLabels[i]->SetLabel((it != m_featureVariables.cend()) ? it->second :
                                                                                wxString{});
            }

        GetSideBarBook()->GetCurrentPage()->Layout();
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertChernoffDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
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
    std::shared_ptr<Data::Dataset> InsertChernoffDlg::GetSelectedDataset() const
        {
        if (GetReportBuilder() == nullptr || m_datasetChoice == nullptr)
            {
            return nullptr;
            }

        const int sel = m_datasetChoice->GetSelection();
        if (sel == wxNOT_FOUND || static_cast<size_t>(sel) >= m_datasetNames.size())
            {
            return nullptr;
            }

        const auto& datasets = GetReportBuilder()->GetDatasets();
        const auto it = datasets.find(m_datasetNames[sel]);
        return (it != datasets.cend()) ? it->second : nullptr;
        }

    //-------------------------------------------
    Gender InsertChernoffDlg::GetGender() const
        {
        return (m_genderChoice->GetSelection() == 1) ? Gender::Male : Gender::Female;
        }

    //-------------------------------------------
    HairStyle InsertChernoffDlg::GetHairStyle() const
        {
        constexpr HairStyle styles[] = { HairStyle::Bald,         HairStyle::Bob,
                                         HairStyle::Pixie,        HairStyle::Bun,
                                         HairStyle::LongStraight, HairStyle::HighTopFade };

        const int sel = m_hairStyleChoice->GetSelection();
        if (sel >= 0 && std::cmp_less(sel, std::size(styles)))
            {
            return styles[sel];
            }
        return HairStyle::Bob;
        }

    //-------------------------------------------
    FacialHair InsertChernoffDlg::GetFacialHair() const
        {
        return (m_facialHairChoice->GetSelection() == 1) ? FacialHair::FiveOClockShadow :
                                                           FacialHair::CleanShaven;
        }

    //-------------------------------------------
    wxString
    InsertChernoffDlg::GetFeatureVariable(Graphs::ChernoffFacesPlot::FeatureId feature) const
        {
        const auto it = m_featureVariables.find(feature);
        return (it != m_featureVariables.cend()) ? it->second : wxString{};
        }

    //-------------------------------------------
    bool InsertChernoffDlg::Validate()
        {
        if (GetSelectedDataset() == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset."), _(L"No Dataset"), wxOK | wxICON_WARNING,
                         this);
            return false;
            }

        using FID = Graphs::ChernoffFacesPlot::FeatureId;
        if (m_featureVariables.find(FID::FaceWidth) == m_featureVariables.cend())
            {
            wxMessageBox(_(L"At least the 'Face Width' variable must be assigned."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING, this);
            return false;
            }

        return true;
        }
    } // namespace Wisteria::UI
