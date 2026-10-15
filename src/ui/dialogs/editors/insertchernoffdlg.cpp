///////////////////////////////////////////////////////////////////////////////
// Name:        insertchernoffdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertchernoffdlg.h"
#include "../../app/wisteriaview.h"
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
    std::vector<DataGuideParameter> InsertChernoffDlg::GetParameters()
        {
        const std::vector<Data::Dataset::ColumnImportType> numericTypes{
            Data::Dataset::ColumnImportType::Numeric
        };
        const std::vector<Data::Dataset::ColumnImportType> categoryTypes{
            Data::Dataset::ColumnImportType::String
        };
        const auto feature = [&numericTypes](const wxString& name, const wxString& effect,
                                             const wxString& group, const bool required = false)
        {
            return DataGuideParameter{ name,
                                       numericTypes,
                                       required ? DataGuideRequirement::Required :
                                                  DataGuideRequirement::Optional,
                                       effect,
                                       {},
                                       false,
                                       wxString{},
                                       group };
        };

        // the order here is the order of the lists in the variable selection dialog
        return {
            feature(_(L"Face width"), _(L"Narrow to wide."), _(L"Face"), true),
            feature(_(L"Face height"), _(L"Short to tall."), _(L"Face")),
            feature(_(L"Eye size"), _(L"Small to large."), _(L"Eyes")),
            feature(_(L"Eye position"), _(L"Low to high on the face."), _(L"Eyes")),
            feature(_(L"Eyebrow slant"), _(L"Angled down to angled up."), _(L"Eyes")),
            feature(_(L"Pupil position"), _(L"Looking left to looking right."), _(L"Eyes")),
            feature(_(L"Nose size"), _(L"Small to large."), _(L"Nose and mouth")),
            feature(_(L"Mouth width"), _(L"Narrow to wide."), _(L"Nose and mouth")),
            feature(_(L"Mouth curvature"), _(L"Frown to smile."), _(L"Nose and mouth")),
            feature(_(L"Face color"), _(L"Pale to saturated, within the skin color range."),
                    _(L"Ears and skin")),
            feature(_(L"Ear size"), _(L"Small to large."), _(L"Ears and skin")),
            { _(L"Hair style"),
              categoryTypes,
              DataGuideRequirement::Optional,
              _(L"Each category is given its own hair style, in the order the categories "
                "first appear. Overrides the hair style chosen in the dialog."),
              {},
              false,
              wxString{},
              _(L"Hair") },
            { _(L"Hair addition"),
              categoryTypes,
              DataGuideRequirement::Optional,
              _(L"Each category is given its own addition, such as a beard or mustache on male "
                "faces and a flower or star on female faces."),
              {},
              false,
              wxString{},
              _(L"Hair") }
        };
        }

    //-------------------------------------------
    DataGuide InsertChernoffDlg::GetDataGuide() const
        {
        DataGuide guide;
        guide.m_title = _(L"Chernoff Faces Plot");
        guide.m_iconName = L"images/chernoffplot.svg";
        guide.m_summary =
            _(L"Draws each observation as a face, with each variable controlling a facial "
              "feature. Similar observations look alike and outliers stand out.");
        guide.m_requirements =
            _(L"One row per face, with a numeric column for each feature to vary. At most 36 "
              "rows are drawn (a 6 by 6 grid). Values are scaled to the range of their column.");
        guide.m_shape = DataGuideShape::Long;
        guide.m_parameters = GetParameters();

        DataGuideExample cities;
        cities.m_title = _(L"Example: comparing cities");
        cities.m_columns = { { _(L"City"), wxString{} },
                             { _(L"Population"), _(L"Face width") },
                             { _(L"Income"), _(L"Face height") },
                             { _(L"Rainfall"), _(L"Eye size") },
                             { _(L"Happiness"), _(L"Mouth curvature") } };
        cities.m_rows = { { L"Akron", L"199", L"48", L"39", L"6.1" },
                          { L"Columbus", L"905", L"62", L"38", L"7.4" },
                          { L"Dayton", L"137", L"45", L"41", L"5.2" } };
        guide.m_examples = { std::move(cities) };

        guide.m_missingData = {
            _(L"Feature columns: a missing value is drawn as the midpoint of the feature. A "
              "row with every feature missing is not drawn."),
            _(L"Hair style and hair addition: a missing category uses the default.")
        };
        guide.m_notes = {
            _(L"Only the first 36 rows are drawn, so filter or aggregate larger datasets first."),
            _(L"Features that are not assigned stay at their default look, so only assign the "
              "variables you want to compare."),
            _(L"Faces are labeled beneath with the dataset's ID column when labels are shown.")
        };
        return guide;
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
        auto* datasetChoice = CreateDatasetChoice(optionsPage);
        datasetSizer->Add(datasetChoice);

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
                             wxGenericValidator{ &m_gender });
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
                                         nullptr, 0, wxGenericValidator{ &m_hairStyle });
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
                                         wxGenericValidator{ &m_showLabels }),
                          wxSizerFlags{}.Border());

        // bind events
        datasetChoice->Bind(wxEVT_CHOICE,
                            [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });

        varButton->Bind(wxEVT_BUTTON,
                        [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateLegendOptionsPage();
        if (GetLegendPage() != nullptr)
            {
            GetLegendPage()->GetSizer()->Add(
                new wxCheckBox(GetLegendPage(), wxID_ANY, _(L"Use enhanced legend"),
                               wxDefaultPosition, wxDefaultSize, 0,
                               wxGenericValidator{ &m_useEnhancedLegend }),
                wxSizerFlags{}.Border());
            }
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

        using FID = Graphs::ChernoffFacesPlot::FeatureId;

        constexpr FID featureOrder[] = { FID::FaceWidth,   FID::FaceHeight,   FID::EyeSize,
                                         FID::EyePosition, FID::EyebrowSlant, FID::PupilDirection,
                                         FID::NoseSize,    FID::MouthWidth,   FID::SmileFrown,
                                         FID::FaceColor,   FID::EarSize,      FID::HairStyle,
                                         FID::HairAddition };

        const auto parameters = GetParameters();
        std::vector<VariableSelectDlg::VariableListInfo> variableLists;
        for (size_t paramIndex = 0; paramIndex < parameters.size(); ++paramIndex)
            {
            const auto foundPos = m_featureVariables.find(featureOrder[paramIndex]);
            const bool isAssigned =
                foundPos != m_featureVariables.cend() && !foundPos->second.empty();
            variableLists.push_back(MakeVariableList(
                parameters[paramIndex],
                isAssigned ? std::vector<wxString>{ foundPos->second } : std::vector<wxString>{}));
            }
        VariableSelectDlg dlg(this, columnInfo, variableLists);

        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        // map feature IDs to selected variable names
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
        SelectDataset(chernoff->GetPropertyTemplate(L"dataset"));

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

    //-------------------------------------------
    std::shared_ptr<Graphs::ChernoffFacesPlot>
    InsertChernoffDlg::BuildChernoffFacesPlot(const Graphs::Graph2D* oldGraph)
        {
        auto plot = std::make_shared<Graphs::ChernoffFacesPlot>(GetCanvas(), GetSkinColorDarker());
        if (oldGraph != nullptr)
            {
            plot->SetId(oldGraph->GetId());
            }
        ApplyGraphOptions(*plot);
        ApplyPageOptions(*plot);
        plot->SetSkinColorRange(GetSkinColorLighter(), GetSkinColorDarker());
        plot->SetGender(GetGender());
        plot->SetHairStyle(GetHairStyleFemale());
        plot->SetHairStyle(GetHairStyleMale());
        plot->SetEyeColor(GetEyeColor());
        plot->SetHairColor(GetHairColor());
        plot->SetLipstickColor(GetLipstickColor());
        plot->ShowLabels(GetShowLabels());

        plot->SetPropertyTemplate(L"enhanced-legend", GetUseEnhancedLegend() ? L"true" : L"false");

        using FID = Graphs::ChernoffFacesPlot::FeatureId;
        const auto optVar = [this](FID id) -> std::optional<wxString>
        {
            const auto var = GetFeatureVariable(id);
            return var.empty() ? std::nullopt : std::optional<wxString>(var);
        };

        plot->SetData(GetSelectedDataset(), GetFeatureVariable(FID::FaceWidth),
                      optVar(FID::FaceHeight), optVar(FID::EyeSize), optVar(FID::EyePosition),
                      optVar(FID::EyebrowSlant), optVar(FID::PupilDirection), optVar(FID::NoseSize),
                      optVar(FID::MouthWidth), optVar(FID::SmileFrown), optVar(FID::FaceColor),
                      optVar(FID::EarSize), optVar(FID::HairStyle), optVar(FID::HairAddition));

        const std::pair<FID, wxString> featureProps[] = {
            { FID::FaceWidth, L"face-width" },       { FID::FaceHeight, L"face-height" },
            { FID::EyeSize, L"eye-size" },           { FID::EyePosition, L"eye-position" },
            { FID::EyebrowSlant, L"eyebrow-slant" }, { FID::PupilDirection, L"pupil-position" },
            { FID::NoseSize, L"nose-size" },         { FID::MouthWidth, L"mouth-width" },
            { FID::SmileFrown, L"mouth-curvature" }, { FID::FaceColor, L"face-saturation" },
            { FID::EarSize, L"ear-size" },           { FID::HairStyle, L"hair-style" },
            { FID::HairAddition, L"hair-addition" }
        };

        if (oldGraph != nullptr)
            {
            ApplyAxisOverrides(*plot);

            const auto* oldChernoff = dynamic_cast<const Graphs::ChernoffFacesPlot*>(oldGraph);
            WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"dataset",
                                               GetSelectedDatasetName(),
                                               oldGraph->GetPropertyTemplate(L"dataset"));
            for (const auto& [fid, propName] : featureProps)
                {
                const auto var = GetFeatureVariable(fid);
                const auto oldExpanded =
                    (oldChernoff != nullptr) ? oldChernoff->GetFeatureColumnName(fid) : wxString{};
                WisteriaView::CarryForwardProperty(*oldGraph, *plot, L"variables." + propName, var,
                                                   oldExpanded);
                }
            }
        else
            {
            plot->SetPropertyTemplate(L"dataset", GetSelectedDatasetName());
            for (const auto& [fid, propName] : featureProps)
                {
                const auto var = GetFeatureVariable(fid);
                if (!var.empty())
                    {
                    plot->SetPropertyTemplate(L"variables." + propName, var);
                    }
                }
            }

        return plot;
        }
    } // namespace Wisteria::UI
