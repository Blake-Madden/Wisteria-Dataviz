///////////////////////////////////////////////////////////////////////////////
// Name:        insertchernoffdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertchernoffdlg.h"
#include "variableselectdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertChernoffDlg::InsertChernoffDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
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
        auto* varsBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Variables"));
        auto* varButton =
            new wxButton(varsBox->GetStaticBox(), ID_SELECT_VARS_BUTTON, _(L"Select..."));
        varsBox->Add(varButton, wxSizerFlags{}.Border(wxLEFT));

        // feature-to-variable label grid
        using FID = Graphs::ChernoffFacesPlot::FeatureId;
        constexpr FID allFeatures[] = { FID::FaceWidth,   FID::FaceHeight,   FID::EyeSize,
                                        FID::EyePosition, FID::EyebrowSlant, FID::PupilDirection,
                                        FID::NoseSize,    FID::MouthWidth,   FID::SmileFrown,
                                        FID::FaceColor,   FID::EarSize };

        auto* featureGrid = new wxFlexGridSizer(2, wxSize{ FromDIP(12), FromDIP(2) });

        for (size_t i = 0; i < FEATURE_COUNT; ++i)
            {
            auto* featureLabel = new wxStaticText(
                varsBox->GetStaticBox(), wxID_ANY,
                Graphs::ChernoffFacesPlot::GetFeatureDisplayName(allFeatures[i]) + L":");
            featureLabel->SetFont(featureLabel->GetFont().Bold());
            featureGrid->Add(featureLabel, wxSizerFlags{}.CenterVertical());

            m_featureVarLabels[i] = new wxStaticText(varsBox->GetStaticBox(), wxID_ANY, wxString{});
            m_featureVarLabels[i]->SetForegroundColour(GetVariableLabelColor());
            featureGrid->Add(m_featureVarLabels[i], wxSizerFlags{}.CenterVertical());
            }

        varsBox->Add(featureGrid, wxSizerFlags{}.Border());
        optionsSizer->Add(varsBox, wxSizerFlags{}.Border());

        // appearance options
        auto* appearanceSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });

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
                                   m_facialHairLabel->Enable(m_gender == 1);
                                   m_facialHairChoice->Enable(m_gender == 1);
                                   Refresh();
                               });
            }

        // hair style
        appearanceSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Hair style:")),
                             wxSizerFlags{}.CenterVertical());
            {
            auto* hairStyleChoice =
                new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, 0,
                             wxGenericValidator(&m_hairStyle));
            hairStyleChoice->Append(_(L"Bald"));
            hairStyleChoice->Append(_(L"Bob"));
            hairStyleChoice->Append(_(L"Pixie"));
            hairStyleChoice->Append(_(L"Bun"));
            hairStyleChoice->Append(_(L"Long straight"));
            hairStyleChoice->Append(_(L"High top fade"));
            appearanceSizer->Add(hairStyleChoice);
            }

        // facial hair
        m_facialHairLabel = new wxStaticText(optionsPage, wxID_ANY, _(L"Facial hair:"));
        appearanceSizer->Add(m_facialHairLabel, wxSizerFlags{}.CenterVertical());
        m_facialHairChoice = new wxChoice(optionsPage, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                          0, nullptr, 0, wxGenericValidator(&m_facialHair));
        m_facialHairChoice->Append(_(L"Clean shaven"));
        m_facialHairChoice->Append(_(L"Five o'clock shadow"));
        m_facialHairLabel->Enable(m_gender == 1);
        m_facialHairChoice->Enable(m_gender == 1);
        appearanceSizer->Add(m_facialHairChoice);

        // skin color range (lighter and darker side by side)
        appearanceSizer->Add(new wxStaticText(optionsPage, wxID_ANY, _(L"Skin color:")),
                             wxSizerFlags{}.CenterVertical());
        auto* skinColorSizer = new wxBoxSizer(wxHORIZONTAL);
        m_skinColorLighterPicker =
            new wxColourPickerCtrl(optionsPage, wxID_ANY, wxColour{ 255, 239, 219 });
        skinColorSizer->Add(m_skinColorLighterPicker, wxSizerFlags{}.CenterVertical());
        skinColorSizer->Add(new wxStaticText(optionsPage, wxID_ANY, L"–"),
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
        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Show labels (from ID column)"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_showLabels)),
                          wxSizerFlags{}.Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY, _(L"Use enhanced legend"),
                                         wxDefaultPosition, wxDefaultSize, 0,
                                         wxGenericValidator(&m_useEnhancedLegend)),
                          wxSizerFlags{}.Border());

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
                                    .Label(_(L"Face Width"))
                                    .SingleSelection(true)
                                    .Required(true)
                                    .DefaultVariables(defaultVar(FID::FaceWidth))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Face Height"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::FaceHeight))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Eye Size"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::EyeSize))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Eye Position"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::EyePosition))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Eyebrow Slant"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::EyebrowSlant))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Pupil Position"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::PupilDirection))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Nose Size"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::NoseSize))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Mouth Width"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::MouthWidth))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Mouth Curvature"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::SmileFrown))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Face Color"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::FaceColor))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
                                VLI{}
                                    .Label(_(L"Ear Size"))
                                    .SingleSelection(true)
                                    .Required(false)
                                    .DefaultVariables(defaultVar(FID::EarSize))
                                    .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }) });

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        // map feature IDs to selected variable names
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
    HairStyle InsertChernoffDlg::GetHairStyle() const
        {
        constexpr HairStyle styles[] = { HairStyle::Bald,         HairStyle::Bob,
                                         HairStyle::Pixie,        HairStyle::Bun,
                                         HairStyle::LongStraight, HairStyle::HighTopFade };

        if (m_hairStyle >= 0 && std::cmp_less(m_hairStyle, std::size(styles)))
            {
            return styles[m_hairStyle];
            }
        return HairStyle::Bob;
        }

    //-------------------------------------------
    FacialHair InsertChernoffDlg::GetFacialHair() const
        {
        return (m_facialHair == 1) ? FacialHair::FiveOClockShadow : FacialHair::CleanShaven;
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
                                    FID::FaceColor,   FID::EarSize };
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
        m_facialHairLabel->Enable(m_gender == 1);
        m_facialHairChoice->Enable(m_gender == 1);

        switch (chernoff->GetHairStyle())
            {
        case HairStyle::Bald:
            m_hairStyle = 0;
            break;
        case HairStyle::Bob:
            m_hairStyle = 1;
            break;
        case HairStyle::Pixie:
            m_hairStyle = 2;
            break;
        case HairStyle::Bun:
            m_hairStyle = 3;
            break;
        case HairStyle::LongStraight:
            m_hairStyle = 4;
            break;
        case HairStyle::HighTopFade:
            m_hairStyle = 5;
            break;
        default:
            m_hairStyle = 1;
            break;
            }

        m_facialHair = (chernoff->GetFacialHair() == FacialHair::FiveOClockShadow) ? 1 : 0;

        // color pickers
        m_skinColorLighterPicker->SetColour(chernoff->GetFaceColorLighter());
        m_skinColorDarkerPicker->SetColour(chernoff->GetFaceColor());
        m_eyeColorPicker->SetColour(chernoff->GetEyeColor());
        m_hairColorPicker->SetColour(chernoff->GetHairColor());

        m_showLabels = chernoff->IsShowingLabels();

        m_useEnhancedLegend =
            (chernoff->GetLastLegendType() == Graphs::ChernoffFacesPlot::LegendType::Enhanced);

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
