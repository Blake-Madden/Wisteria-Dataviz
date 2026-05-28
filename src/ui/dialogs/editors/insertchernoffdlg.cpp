///////////////////////////////////////////////////////////////////////////////
// Name:        insertchernoffdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertchernoffdlg.h"
#include "../variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertChernoffDlg::InsertChernoffDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                         wxWindow* parent, const wxString& caption,
                                         const wxWindowID id, const wxPoint& pos,
                                         const wxSize& size, const long style, EditMode editMode)
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
    void InsertChernoffDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Chernoff Faces"), ID_OPTIONS_SECTION, true);

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

        // feature-to-variable label grid
        using FID = Graphs::ChernoffFacesPlot::FeatureId;
        constexpr FID allFeatures[] = { FID::FaceWidth,   FID::FaceHeight,   FID::EyeSize,
                                        FID::EyePosition, FID::EyebrowSlant, FID::PupilDirection,
                                        FID::NoseSize,    FID::MouthWidth,   FID::SmileFrown,
                                        FID::FaceColor,   FID::EarSize,      FID::HairStyle,
                                        FID::HairAddition };

        auto* featureGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(12), FromDIP(2) });

        for (size_t i = 0; i < FEATURE_COUNT; ++i)
            {
            auto* featureLabel = new wxStaticText(
                varsBox->GetStaticBox(), wxID_ANY,
                Graphs::ChernoffFacesPlot::GetFeatureDisplayName(allFeatures[i]) + L":");
            featureLabel->SetFont(featureLabel->GetFont().Bold());
            featureGrid->Add(featureLabel, wxSizerFlags{}.CenterVertical());

            m_featureVarLabels[i] = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
            m_featureVarLabels[i]->SetForegroundColour(
                Wisteria::Settings::GetHighlightedLabelColor());
            featureGrid->Add(m_featureVarLabels[i], wxSizerFlags{}.CenterVertical());
            }

        varsBox->Add(featureGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // appearance options
        auto* appearanceSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        // gender
        appearanceSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Gender:")),
                             wxSizerFlags{}.CenterVertical());
            {
            auto* genderChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_gender));
            genderChoice->Append(_(L"Female"));
            genderChoice->Append(_(L"Male"));
            appearanceSizer->Add(genderChoice);

            genderChoice->Bind(wxEVT_CHOICE,
                               [this]([[maybe_unused]] wxCommandEvent&)
                               {
                                   TransferDataFromWindow();
                                   m_lipstickColorLabel->Enable(m_gender == 0);
                                   m_lipstickColorPicker->Enable(m_gender == 0);
                                   PopulateHairStyleChoice();
                                   Refresh();
                               });
            }

        // Hair style (disabled when a categorical variable is mapped to FID::HairStyle).
        // Labels are populated based on gender via PopulateHairStyleChoice().
        m_hairStyleLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"Hair style:"));
        appearanceSizer->Add(m_hairStyleLabel, wxSizerFlags{}.CenterVertical());
        m_hairStyleChoice = new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0,
                                         nullptr, 0, wxGenericValidator(&m_hairStyle));
        PopulateHairStyleChoice();
        appearanceSizer->Add(m_hairStyleChoice);

        // skin color range (lighter and darker side by side)
        appearanceSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Skin color:")),
                             wxSizerFlags{}.CenterVertical());
        auto* skinColorSizer = new wxBoxSizer(wxHORIZONTAL);
        m_skinColorLighterPicker =
            new wxColourPickerCtrl(optionsPage, wxID_ANY, wxColour{ 255, 239, 219 });
        skinColorSizer->Add(m_skinColorLighterPicker, wxSizerFlags{}.CenterVertical());
        skinColorSizer->Add(new wxStaticText(optionsPage, wxID_ANY, L"–"),
                            wxSizerFlags{}.CenterVertical().Border(
                                wxLEFT | wxRIGHT, wxSizerFlags::GetDefaultBorder()));
        m_skinColorDarkerPicker =
            new wxColourPickerCtrl(optionsPage, wxID_ANY, wxColour{ 255, 224, 189 });
        skinColorSizer->Add(m_skinColorDarkerPicker, wxSizerFlags{}.CenterVertical());
        appearanceSizer->Add(skinColorSizer);

        optionsSizer->Add(appearanceSizer, wxSizerFlags{}.Border());

        // cosmetic options
        auto* cosmeticBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Cosmetic"));
        auto* cosmeticGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        // eye color
        cosmeticGrid->Add(new wxStaticText(cosmeticBox->GetStaticBox(), wxID_ANY, _(L"Eye color:")),
                          wxSizerFlags{}.CenterVertical());
        m_eyeColorPicker = new wxColourPickerCtrl(cosmeticBox->GetStaticBox(), wxID_ANY,
                                                  wxColour{ 143, 188, 143 });
        cosmeticGrid->Add(m_eyeColorPicker);

        // hair color
        cosmeticGrid->Add(
            new wxStaticText(cosmeticBox->GetStaticBox(), wxID_ANY, _(L"Hair color:")),
            wxSizerFlags{}.CenterVertical());
        m_hairColorPicker =
            new wxColourPickerCtrl(cosmeticBox->GetStaticBox(), wxID_ANY, wxColour{ 183, 82, 46 });
        cosmeticGrid->Add(m_hairColorPicker);

        // lipstick color (female only)
        m_lipstickColorLabel =
            new wxStaticText(cosmeticBox->GetStaticBox(), wxID_ANY, _(L"Lipstick color:"));
        cosmeticGrid->Add(m_lipstickColorLabel, wxSizerFlags{}.CenterVertical());
        m_lipstickColorPicker =
            new wxColourPickerCtrl(cosmeticBox->GetStaticBox(), wxID_ANY, wxColour{ 178, 34, 34 });
        m_lipstickColorLabel->Enable(m_gender == 0);
        m_lipstickColorPicker->Enable(m_gender == 0);
        cosmeticGrid->Add(m_lipstickColorPicker);

        cosmeticBox->Add(cosmeticGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(cosmeticBox, wxSizerFlags{}.Border().Expand());

        // show labels
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show labels (from ID column)"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showLabels)),
                          wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Use enhanced legend"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_useEnhancedLegend)),
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

        CreateGraphOptionsPage();
        CreatePageOptionsPage();
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
        using FID = Graphs::ChernoffFacesPlot::FeatureId;

        const auto defaultVar = [this](FID fid) -> std::vector<wxString>
        {
            const auto it = m_featureVariables.find(fid);
            if (it != m_featureVariables.cend() && !it->second.empty())
                {
                return { it->second };
                }
            return {};
        };

        VariableSelectDlg dlg(this, columnInfo,
                              { VLI{}
                                    .Label(_(L"Face width"))
                                    .SingleSelection(true)
                                    .Required(true)
                                    .DefaultVariables(defaultVar(FID::FaceWidth))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Face height"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::FaceHeight))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Eye size"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::EyeSize))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Eye position"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::EyePosition))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Eyebrow slant"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::EyebrowSlant))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Pupil position"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::PupilDirection))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Nose size"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::NoseSize))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Mouth width"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::MouthWidth))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Mouth curvature"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::SmileFrown))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Face color"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::FaceColor))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Ear size"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::EarSize))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Hair style"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::HairStyle))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::String }),
                                VLI{}
                                    .Label(_(L"Hair addition"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::HairAddition))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::String }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        // map feature IDs to selected variable names
        constexpr FID featureOrder[] = { FID::FaceWidth,   FID::FaceHeight,   FID::EyeSize,
                                         FID::EyePosition, FID::EyebrowSlant, FID::PupilDirection,
                                         FID::NoseSize,    FID::MouthWidth,   FID::SmileFrown,
                                         FID::FaceColor,   FID::EarSize,      FID::HairStyle,
                                         FID::HairAddition };

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

        // a categorical hair-style variable overrides the static choice control;
        // disable the choice so its value can't conflict with the mapped column
        const bool hairStyleVarMapped =
            m_featureVariables.find(FID::HairStyle) != m_featureVariables.cend();
        m_hairStyleLabel->Enable(!hairStyleVarMapped);
        m_hairStyleChoice->Enable(!hairStyleVarMapped);
        }

    //-------------------------------------------
    void InsertChernoffDlg::UpdateFeatureLabels()
        {
        using FID = Graphs::ChernoffFacesPlot::FeatureId;
        constexpr FID allFeatures[] = { FID::FaceWidth,   FID::FaceHeight,   FID::EyeSize,
                                        FID::EyePosition, FID::EyebrowSlant, FID::PupilDirection,
                                        FID::NoseSize,    FID::MouthWidth,   FID::SmileFrown,
                                        FID::FaceColor,   FID::EarSize,      FID::HairStyle,
                                        FID::HairAddition };

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
        if (sel == wxNOT_FOUND || std::cmp_greater_equal(sel, m_datasetNames.size()))
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
        return (m_gender == 1) ? Gender::Male : Gender::Female;
        }

    //-------------------------------------------
    void InsertChernoffDlg::PopulateHairStyleChoice()
        {
        if (m_hairStyleChoice == nullptr)
            {
            return;
            }
        // m_hairStyle is the source of truth; the choice is rebuilt from it.
        // Label order must mirror HairStyleFemale / HairStyleMale enum order.
        m_hairStyleChoice->Clear();
        if (m_gender == 1)
            {
            m_hairStyleChoice->Append(_(L"Comb-over"));
            m_hairStyleChoice->Append(_(L"High top fade"));
            m_hairStyleChoice->Append(_(L"Flat top"));
            m_hairStyleChoice->Append(_(L"Partially bald"));
            m_hairStyleChoice->Append(_(L"Bald comb-over"));
            m_hairStyleChoice->Append(_(L"Long & straight"));
            m_hairStyleChoice->Append(_(L"Bald"));
            m_hairStyleChoice->Append(_(L"Curly"));
            m_hairStyleChoice->Append(_(L"Long & curly"));
            }
        else
            {
            m_hairStyleChoice->Append(_(L"Bob"));
            m_hairStyleChoice->Append(_(L"Pixie"));
            m_hairStyleChoice->Append(_(L"Bun"));
            m_hairStyleChoice->Append(_(L"Long & straight"));
            m_hairStyleChoice->Append(_(L"Curly"));
            m_hairStyleChoice->Append(_(L"Long & curly"));
            m_hairStyleChoice->Append(_(L"High top fade"));
            m_hairStyleChoice->Append(_(L"Flat top"));
            m_hairStyleChoice->Append(_(L"Bald"));
            }

        if (m_hairStyle < 0 || std::cmp_greater_equal(m_hairStyle, m_hairStyleChoice->GetCount()))
            {
            m_hairStyle = 0;
            }
        m_hairStyleChoice->SetSelection(m_hairStyle);
        }

    //-------------------------------------------
    HairStyleFemale InsertChernoffDlg::GetHairStyleFemale() const
        {
        if (m_hairStyle >= 0 &&
            std::cmp_less(m_hairStyle, static_cast<int>(HairStyleFemale::FEMALE_HAIR_STYLE_COUNT)))
            {
            return static_cast<HairStyleFemale>(m_hairStyle);
            }
        return HairStyleFemale::Bob;
        }

    //-------------------------------------------
    HairStyleMale InsertChernoffDlg::GetHairStyleMale() const
        {
        if (m_hairStyle >= 0 &&
            std::cmp_less(m_hairStyle, static_cast<int>(HairStyleMale::MALE_HAIR_STYLE_COUNT)))
            {
            return static_cast<HairStyleMale>(m_hairStyle);
            }
        return HairStyleMale::PartiallyBald;
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
            OnSelectVariables();
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertChernoffDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* chernoff = dynamic_cast<const Graphs::ChernoffFacesPlot*>(&graph);
        if (chernoff == nullptr)
            {
            return;
            }

        // load graph and page options from the base classes
        LoadGraphOptions(graph);

        // select the dataset by name from the property template
        const auto dsName = chernoff->GetPropertyTemplate(L"dataset");
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

        // load actual column names from the graph
        // (property templates may contain unexpanded {{placeholders}})
        using FID = Graphs::ChernoffFacesPlot::FeatureId;
        const FID allFeatures[] = { FID::FaceWidth,   FID::FaceHeight,   FID::EyeSize,
                                    FID::EyePosition, FID::EyebrowSlant, FID::PupilDirection,
                                    FID::NoseSize,    FID::MouthWidth,   FID::SmileFrown,
                                    FID::FaceColor,   FID::EarSize,      FID::HairStyle,
                                    FID::HairAddition };
        m_featureVariables.clear();
        for (const auto fid : allFeatures)
            {
            const auto var = chernoff->GetFeatureColumnName(fid);
            if (!var.empty())
                {
                m_featureVariables[fid] = var;
                }
            }
        UpdateFeatureLabels();

        // appearance options
        m_gender = (chernoff->GetGender() == Gender::Male) ? 1 : 0;
        m_lipstickColorLabel->Enable(m_gender == 0);
        m_lipstickColorPicker->Enable(m_gender == 0);

        // mirror the variable-mapping rule: hair-style choice is disabled when a
        // categorical variable is mapped to FID::HairStyle
        const bool hairStyleVarMapped =
            m_featureVariables.find(FID::HairStyle) != m_featureVariables.cend();
        m_hairStyleLabel->Enable(!hairStyleVarMapped);
        m_hairStyleChoice->Enable(!hairStyleVarMapped);

        // parallel-index mapping: HairStyleFemale and HairStyleMale share the same
        // integer layout, so the combobox index is just the underlying enum value
        m_hairStyle = (chernoff->GetGender() == Gender::Male) ?
                          static_cast<int>(chernoff->GetHairStyleMale()) :
                          static_cast<int>(chernoff->GetHairStyleFemale());

        // rebuild the choice with the loaded gender's labels; this also syncs
        // the combobox selection to m_hairStyle
        PopulateHairStyleChoice();

        // color pickers
        m_skinColorLighterPicker->SetColour(chernoff->GetFaceColorLighter());
        m_skinColorDarkerPicker->SetColour(chernoff->GetFaceColor());
        m_eyeColorPicker->SetColour(chernoff->GetEyeColor());
        m_hairColorPicker->SetColour(chernoff->GetHairColor());
        m_lipstickColorPicker->SetColour(chernoff->GetLipstickColor());

        m_showLabels = chernoff->IsShowingLabels();

        m_useEnhancedLegend =
            (chernoff->GetLastLegendType() == Graphs::ChernoffFacesPlot::LegendType::Enhanced);

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
