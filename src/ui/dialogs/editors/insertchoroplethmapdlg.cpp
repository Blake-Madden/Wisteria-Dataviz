///////////////////////////////////////////////////////////////////////////////
// Name:        insertchoroplethmapdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertchoroplethmapdlg.h"
#include "../../../graphs/choroplethmap.h"
#include "../variableselectdlg.h"
#include <utility>
#include <vector>
#include <wx/filename.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    int InsertChoroplethMapDlg::NoDataFillStyleToChoiceIndex(const wxBrushStyle style)
        {
        switch (style)
            {
        case wxBRUSHSTYLE_FDIAGONAL_HATCH:
            return 1;
        case wxBRUSHSTYLE_BDIAGONAL_HATCH:
            return 2;
        case wxBRUSHSTYLE_CROSSDIAG_HATCH:
            return 3;
        case wxBRUSHSTYLE_CROSS_HATCH:
            return 4;
        case wxBRUSHSTYLE_HORIZONTAL_HATCH:
            return 5;
        case wxBRUSHSTYLE_VERTICAL_HATCH:
            return 6;
        default:
            return 0;
            }
        }

    //-------------------------------------------
    InsertChoroplethMapDlg::InsertChoroplethMapDlg(Canvas* canvas,
                                                   const ReportBuilder* reportBuilder,
                                                   wxWindow* parent, const wxString& caption,
                                                   const wxWindowID id, const wxPoint& pos,
                                                   const wxSize& size, const long style,
                                                   EditMode editMode)
        : InsertGraphDlg(canvas, reportBuilder, parent, caption, id, pos, size, style, editMode)
        {
        CreateControls();
        FinalizeControls();

        if (m_datasetChoice != nullptr)
            {
            m_datasetChoice->Enable(true);
            }

        SetMinSize(GetSize());

        Centre();
        }

    //-------------------------------------------
    void InsertChoroplethMapDlg::CreateControls()
        {
        InsertGraphDlg::CreateControls();

        auto* optionsPage = new wxPanel(GetSideBarBook());
        auto* optionsSizer = new wxBoxSizer(wxVERTICAL);
        optionsPage->SetSizer(optionsSizer);
        GetSideBarBook()->AddPage(optionsPage, _(L"Choropleth Map"), ID_OPTIONS_SECTION, true);

        // KML file
        auto* kmlBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Regions (KML file)"));
        m_kmlPicker = new wxFilePickerCtrl(
            kmlBox->GetStaticBox(), wxID_ANY, wxString{}, _(L"Select a KML file"),
            _(L"KML files (*.kml)|*.kml|All files (*.*)|*.*"), wxDefaultPosition, wxDefaultSize,
            wxFLP_DEFAULT_STYLE | wxFLP_USE_TEXTCTRL);
        kmlBox->Add(m_kmlPicker, wxSizerFlags{ 1 }.Expand().Border());
        m_kmlPicker->Bind(wxEVT_FILEPICKER_CHANGED,
                          [this]([[maybe_unused]]
                                 wxFileDirPickerEvent& event) { OnKMLFileChanged(); });

        auto* idFieldSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        idFieldSizer->Add(
            new wxStaticText(kmlBox->GetStaticBox(), wxID_ANY, _(L"Region key field:")),
            wxSizerFlags{}.CenterVertical());
        m_kmlIdFieldCombo =
            new wxComboBox(kmlBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, 0, nullptr, 0, wxGenericValidator(&m_kmlIdField));
        m_kmlIdFieldCombo->SetHint(_(L"(placemark name)"));
        idFieldSizer->Add(m_kmlIdFieldCombo, wxSizerFlags{}.Expand());
        kmlBox->Add(idFieldSizer, wxSizerFlags{}.Expand().Border());

        optionsSizer->Add(kmlBox, wxSizerFlags{}.Expand().Border());

        // optional data to shade by
        auto* dataBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Data to map (optional)"));
        auto* dataGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        dataGrid->Add(new wxStaticText(dataBox->GetStaticBox(), wxID_ANY, _(L"Dataset:")),
                      wxSizerFlags{}.CenterVertical());
        m_datasetChoice = new wxChoice(dataBox->GetStaticBox(), ID_DATASET_CHOICE);
        m_datasetChoice->Append(_(L"(none)"));
        if (GetReportBuilder() != nullptr)
            {
            for (const auto& [name, dataset] : GetReportBuilder()->GetDatasets())
                {
                m_datasetNames.push_back(name);
                m_datasetChoice->Append(name);
                }
            }
        m_datasetChoice->SetSelection(0);
        dataGrid->Add(m_datasetChoice, wxSizerFlags{}.Expand());

        m_selectVarsButton =
            new wxButton(dataBox->GetStaticBox(), ID_SELECT_VARS_BUTTON, _(L"Select columns..."));
        dataGrid->AddSpacer(0);
        dataGrid->Add(m_selectVarsButton, wxSizerFlags{}.Align(wxALIGN_LEFT));

        const auto addVarRow = [&](const wxString& caption) -> wxStaticText*
        {
            auto* nameLabel = new wxStaticText(dataBox->GetStaticBox(), wxID_ANY, caption);
            nameLabel->SetFont(nameLabel->GetFont().Bold());
            dataGrid->Add(nameLabel, wxSizerFlags{}.CenterVertical());
            auto* valueLabel = new wxStaticText(dataBox->GetStaticBox(), wxID_ANY, wxString{});
            valueLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
            dataGrid->Add(valueLabel, wxSizerFlags{}.CenterVertical());
            return valueLabel;
        };
        m_keyColumnLabel = addVarRow(_(L"Key column:"));
        m_valueColumnLabel = addVarRow(_(L"Value column:"));
        m_categoryColumnLabel = addVarRow(_(L"Category column:"));
        m_symbolColumnLabel = addVarRow(_(L"Symbol size column:"));

        dataBox->Add(dataGrid, wxSizerFlags{}.Expand().Border());
        optionsSizer->Add(dataBox, wxSizerFlags{}.Expand().Border());

        // how a value column is split into classes
        auto* classBox =
            new wxStaticBoxSizer(wxHORIZONTAL, optionsPage, _(L"Value classification"));
            {
            // the order of these entries is the numeric order of
            // ChoroplethMap::ClassificationMethod
            m_classificationChoice =
                new wxChoice(classBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator(&m_classificationMethod));
            m_classificationChoice->Append(_(L"None (continuous ramp)"));
            m_classificationChoice->Append(_(L"Jenks natural breaks"));
            m_classificationChoice->SetSelection(m_classificationMethod);
            classBox->Add(m_classificationChoice, wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
            m_classificationChoice->Bind(wxEVT_CHOICE, [this]([[maybe_unused]] wxCommandEvent&)
                                         { UpdateClassificationControls(); });
            }
        m_classCountLabel =
            new wxStaticText(classBox->GetStaticBox(), wxID_ANY, _(L"Number of classes:"));
        classBox->Add(m_classCountLabel, wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_classCountSpin = new wxSpinCtrl(classBox->GetStaticBox(), wxID_ANY);
        m_classCountSpin->SetRange(2, 12);
        m_classCountSpin->SetValidator(wxGenericValidator(&m_classCount));
        classBox->Add(m_classCountSpin, wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(classBox, wxSizerFlags{}.Expand().Border());

        auto* symbolBox =
            new wxStaticBoxSizer(wxHORIZONTAL, optionsPage, _(L"Proportional symbols"));
        m_symbolColorLabel =
            new wxStaticText(symbolBox->GetStaticBox(), wxID_ANY, _(L"Symbol color:"));
        symbolBox->Add(m_symbolColorLabel, wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        m_symbolColorPicker =
            new wxColourPickerCtrl(symbolBox->GetStaticBox(), wxID_ANY, m_symbolColor);
        m_symbolColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, [this](wxColourPickerEvent& evt)
                                  { m_symbolColor = evt.GetColour(); });
        symbolBox->Add(m_symbolColorPicker, wxSizerFlags{}.CenterVertical());
        optionsSizer->Add(symbolBox, wxSizerFlags{}.Expand().Border());

        auto* labelsBox = new wxStaticBoxSizer(wxVERTICAL, optionsPage, _(L"Labels"));
        labelsBox->Add(new wxCheckBox(labelsBox->GetStaticBox(), wxID_ANY,
                                      _(L"Show region labels on the map"), wxDefaultPosition,
                                      wxDefaultSize, 0, wxGenericValidator(&m_showLabels)),
                       wxSizerFlags{}.Border());

        auto* labelContentSizer = new wxBoxSizer(wxHORIZONTAL);
        labelContentSizer->Add(
            new wxStaticText(labelsBox->GetStaticBox(), wxID_ANY, _(L"Label content:")),
            wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
            {
            // the order of these entries is the numeric order of BinLabelDisplay
            auto* labelContentChoice =
                new wxChoice(labelsBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator{ &m_labelDisplay });
            labelContentChoice->Append(_(L"Value"));
            labelContentChoice->Append(_(L"Percentage"));
            labelContentChoice->Append(_(L"Value and percentage"));
            labelContentChoice->Append(_(L"No label"));
            labelContentChoice->Append(_(L"Region name"));
            labelContentChoice->Append(_(L"Region name and value"));
            labelContentChoice->Append(_(L"Region name and percentage"));
            labelContentSizer->Add(labelContentChoice, wxSizerFlags{}.CenterVertical());
            }
        labelsBox->Add(labelContentSizer, wxSizerFlags{}.Border());

        optionsSizer->Add(labelsBox, wxSizerFlags{}.Expand().Border());

        auto* noDataBox =
            new wxStaticBoxSizer(wxHORIZONTAL, optionsPage, _(L"Regions with no data"));
        noDataBox->Add(new wxStaticText(noDataBox->GetStaticBox(), wxID_ANY, _(L"Fill:")),
                       wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
            {
            auto* noDataFillChoice =
                new wxChoice(noDataBox->GetStaticBox(), wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             0, nullptr, 0, wxGenericValidator{ &m_noDataFillStyle });
            noDataFillChoice->Append(_(L"Solid color"));
            noDataFillChoice->Append(_(L"Forward diagonal hatch"));
            noDataFillChoice->Append(_(L"Backward diagonal hatch"));
            noDataFillChoice->Append(_(L"Crossed diagonal hatch"));
            noDataFillChoice->Append(_(L"Crossed hatch"));
            noDataFillChoice->Append(_(L"Horizontal hatch"));
            noDataFillChoice->Append(_(L"Vertical hatch"));
            noDataBox->Add(noDataFillChoice, wxSizerFlags{}.CenterVertical());
            }
        optionsSizer->Add(noDataBox, wxSizerFlags{}.Expand().Border());

        optionsSizer->Add(new wxCheckBox(optionsPage, wxID_ANY,
                                         _(L"Show latitude and longitude grid"), wxDefaultPosition,
                                         wxDefaultSize, 0, wxGenericValidator{ &m_showGraticule }),
                          wxSizerFlags{}.Border());

        m_datasetChoice->Bind(wxEVT_CHOICE,
                              [this]([[maybe_unused]] wxCommandEvent&) { OnDatasetChanged(); });
        m_selectVarsButton->Bind(wxEVT_BUTTON,
                                 [this]([[maybe_unused]] wxCommandEvent&) { OnSelectVariables(); });

        CreateLegendOptionsPage();

        UpdateVariableLabels();

        CreateGraphOptionsPage();
        CreatePageOptionsPage();
        }

    //-------------------------------------------
    void InsertChoroplethMapDlg::OnDatasetChanged()
        {
        // the previously chosen columns belong to the dataset that was just replaced
        m_keyColumn.clear();
        m_valueColumn.clear();
        m_categoryColumn.clear();
        m_symbolColumn.clear();
        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertChoroplethMapDlg::OnSelectVariables()
        {
        const auto dataset = GetSelectedDataset();
        if (dataset == nullptr)
            {
            wxMessageBox(_(L"Please select a dataset first."), _(L"No Dataset"),
                         wxOK | wxICON_INFORMATION, this);
            return;
            }

        const std::vector<Data::Dataset::ColumnImportType> textTypes{
            Data::Dataset::ColumnImportType::String, Data::Dataset::ColumnImportType::Discrete,
            Data::Dataset::ColumnImportType::DichotomousString,
            Data::Dataset::ColumnImportType::DichotomousDiscrete
        };
        const auto asDefault = [](const wxString& name)
        { return name.empty() ? std::vector<wxString>{} : std::vector<wxString>{ name }; };

        using VLI = VariableSelectDlg::VariableListInfo;
        VariableSelectDlg selectDlg(
            this, BuildColumnPreviewInfo(*dataset),
            { VLI{}
                  .Label(_(L"Key column"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(asDefault(m_keyColumn))
                  .AcceptedTypes(textTypes),
              VLI{}
                  .Label(_(L"Value column (color gradient)"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(asDefault(m_valueColumn))
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }),
              VLI{}
                  .Label(_(L"Category column (color per category)"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(asDefault(m_categoryColumn))
                  .AcceptedTypes(textTypes),
              VLI{}
                  .Label(_(L"Proportional symbol size"))
                  .SingleSelection(true)
                  .Required(false)
                  .DefaultVariables(asDefault(m_symbolColumn))
                  .AcceptedTypes({ Data::Dataset::ColumnImportType::Numeric }) });

        if (selectDlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto keyVars = selectDlg.GetSelectedVariables(0);
        m_keyColumn = keyVars.empty() ? wxString{} : keyVars.front();
        const auto valueVars = selectDlg.GetSelectedVariables(1);
        m_valueColumn = valueVars.empty() ? wxString{} : valueVars.front();
        const auto categoryVars = selectDlg.GetSelectedVariables(2);
        m_categoryColumn = categoryVars.empty() ? wxString{} : categoryVars.front();
        const auto symbolVars = selectDlg.GetSelectedVariables(3);
        m_symbolColumn = symbolVars.empty() ? wxString{} : symbolVars.front();

        if (!m_valueColumn.empty() && !m_categoryColumn.empty())
            {
            wxMessageBox(_(L"A value column and a category column were both chosen. "
                           "Clear one of them before continuing."),
                         _(L"Two Shading Columns Chosen"), wxOK | wxICON_INFORMATION, this);
            }

        UpdateVariableLabels();
        }

    //-------------------------------------------
    void InsertChoroplethMapDlg::UpdateVariableLabels()
        {
        if (m_selectVarsButton != nullptr)
            {
            m_selectVarsButton->Enable(GetSelectedDataset() != nullptr);
            }

        // the legend carries the shading scale and the proportional size key,
        // so it is offered when either a shading column or a symbol column is mapped
        EnableLegendPlacement(IsMappingData() || IsUsingProportionalSymbols());

        const auto setColumnLabel = [](wxStaticText* label, const wxString& text)
        {
            if (label != nullptr)
                {
                label->SetLabel(text);
                label->Refresh();
                }
        };
        setColumnLabel(m_keyColumnLabel, m_keyColumn);
        setColumnLabel(m_valueColumnLabel, m_valueColumn);
        setColumnLabel(m_categoryColumnLabel, m_categoryColumn);
        setColumnLabel(m_symbolColumnLabel, m_symbolColumn);

        // the symbol color only matters once a symbol size column is mapped
        const bool hasSymbolColumn = !m_symbolColumn.empty();
        if (m_symbolColorLabel != nullptr)
            {
            m_symbolColorLabel->Enable(hasSymbolColumn);
            m_symbolColorLabel->Refresh();
            }
        if (m_symbolColorPicker != nullptr)
            {
            m_symbolColorPicker->Enable(hasSymbolColumn);
            }

        UpdateClassificationControls();

        if (GetSideBarBook() != nullptr && GetSideBarBook()->GetCurrentPage() != nullptr)
            {
            GetSideBarBook()->GetCurrentPage()->Layout();
            GetSideBarBook()->GetCurrentPage()->Refresh();
            }
        }

    //-------------------------------------------
    void InsertChoroplethMapDlg::UpdateClassificationControls()
        {
        // classification only applies to a continuous value column
        const bool hasValueColumn = !m_valueColumn.empty();
        if (m_classificationChoice != nullptr)
            {
            m_classificationChoice->Enable(hasValueColumn);
            }

        const bool countEnabled = hasValueColumn && m_classificationChoice != nullptr &&
                                  m_classificationChoice->GetSelection() > 0;
        if (m_classCountLabel != nullptr)
            {
            m_classCountLabel->Enable(countEnabled);
            m_classCountLabel->Refresh();
            }
        if (m_classCountSpin != nullptr)
            {
            m_classCountSpin->Enable(countEnabled);
            }
        }

    //-------------------------------------------
    Data::Dataset::ColumnPreviewInfo
    InsertChoroplethMapDlg::BuildColumnPreviewInfo(const Data::Dataset& dataset) const
        {
        Data::Dataset::ColumnPreviewInfo info;

        if (!dataset.GetIdColumn().GetName().empty())
            {
            info.emplace_back(dataset.GetIdColumn().GetName(),
                              Data::Dataset::ColumnImportType::String, wxString{});
            }
        for (const auto& column : dataset.GetContinuousColumns())
            {
            info.emplace_back(column.GetName(), Data::Dataset::ColumnImportType::Numeric,
                              wxString{});
            }
        for (const auto& column : dataset.GetCategoricalColumns())
            {
            info.emplace_back(column.GetName(), Data::Dataset::ColumnImportType::String,
                              wxString{});
            }

        return info;
        }

    //-------------------------------------------
    void InsertChoroplethMapDlg::OnKMLFileChanged() { PopulateKeyFieldChoices(GetKMLPath()); }

    //-------------------------------------------
    void InsertChoroplethMapDlg::PopulateKeyFieldChoices(const wxString& kmlPath)
        {
        if (m_kmlIdFieldCombo == nullptr)
            {
            return;
            }
        // Clear() drops the typed text along with the list, so save and restore it
        const wxString currentField = m_kmlIdFieldCombo->GetValue();
        m_kmlIdFieldCombo->Clear();
        if (!kmlPath.empty() && wxFileName::FileExists(kmlPath))
            {
            for (const auto& fieldName : Data::KmlReader::ReadFieldNames(kmlPath))
                {
                m_kmlIdFieldCombo->Append(fieldName);
                }
            }

        // if nothing has been chosen yet, prefer a "GEOID" field when the file has one
        if (currentField.empty())
            {
            const int geoIdField = m_kmlIdFieldCombo->FindString(L"GEOID", false);
            if (geoIdField != wxNOT_FOUND)
                {
                m_kmlIdFieldCombo->SetValue(m_kmlIdFieldCombo->GetString(geoIdField));
                return;
                }
            }
        m_kmlIdFieldCombo->SetValue(currentField);
        }

    //-------------------------------------------
    wxString InsertChoroplethMapDlg::GetKMLPath() const
        {
        return (m_kmlPicker != nullptr) ? m_kmlPicker->GetPath() : wxString{};
        }

    //-------------------------------------------
    std::shared_ptr<Data::Dataset> InsertChoroplethMapDlg::GetSelectedDataset() const
        {
        if (GetReportBuilder() == nullptr || m_datasetChoice == nullptr)
            {
            return nullptr;
            }
        // index 0 is the "(none)" entry
        const int sel = m_datasetChoice->GetSelection() - 1;
        if (sel < 0 || std::cmp_greater_equal(sel, m_datasetNames.size()))
            {
            return nullptr;
            }
        const auto& datasets = GetReportBuilder()->GetDatasets();
        const auto foundDataset = datasets.find(m_datasetNames[sel]);
        return (foundDataset != datasets.cend()) ? foundDataset->second : nullptr;
        }

    //-------------------------------------------
    wxString InsertChoroplethMapDlg::GetSelectedDatasetName() const
        {
        const int sel = (m_datasetChoice != nullptr) ? m_datasetChoice->GetSelection() - 1 : -1;
        return (sel >= 0 && std::cmp_less(sel, m_datasetNames.size())) ? m_datasetNames[sel] :
                                                                         wxString{};
        }

    //-------------------------------------------
    wxString InsertChoroplethMapDlg::GetKeyColumn() const { return m_keyColumn; }

    //-------------------------------------------
    wxString InsertChoroplethMapDlg::GetValueColumn() const { return m_valueColumn; }

    //-------------------------------------------
    wxString InsertChoroplethMapDlg::GetCategoryColumn() const { return m_categoryColumn; }

    //-------------------------------------------
    wxString InsertChoroplethMapDlg::GetSymbolColumn() const { return m_symbolColumn; }

    //-------------------------------------------
    bool InsertChoroplethMapDlg::IsUsingProportionalSymbols() const
        {
        return !m_symbolColumn.empty() && GetSelectedDataset() != nullptr;
        }

    //-------------------------------------------
    bool InsertChoroplethMapDlg::IsMappingData() const
        {
        return (GetSelectedDataset() != nullptr) &&
               (!m_valueColumn.empty() || !m_categoryColumn.empty());
        }

    //-------------------------------------------
    bool InsertChoroplethMapDlg::IsShowingRegionLabels() const { return m_showLabels; }

    //-------------------------------------------
    bool InsertChoroplethMapDlg::Validate()
        {
        const wxString kmlPath = GetKMLPath();
        if (kmlPath.empty() || !wxFileName::FileExists(kmlPath))
            {
            wxMessageBox(_(L"Please select a KML file for the region shapes."),
                         _(L"KML File Not Specified"), wxOK | wxICON_WARNING, this);
            return false;
            }

        if (!GetValueColumn().empty() && !GetCategoryColumn().empty())
            {
            wxMessageBox(_(L"Please choose either a value column or a category column, not both."),
                         _(L"Two Shading Columns Chosen"), wxOK | wxICON_WARNING, this);
            return false;
            }

        if (GetSelectedDataset() != nullptr &&
            (!GetValueColumn().empty() || !GetCategoryColumn().empty() ||
             !GetSymbolColumn().empty()) &&
            GetKeyColumn().empty())
            {
            wxMessageBox(_(L"Please choose the dataset column that matches the region key."),
                         _(L"Key Column Not Specified"), wxOK | wxICON_WARNING, this);
            return false;
            }

        if (!ValidateColorScheme())
            {
            return false;
            }

        return true;
        }

    //-------------------------------------------
    void InsertChoroplethMapDlg::LoadFromGraph(const Graphs::Graph2D& graph)
        {
        const auto* choroplethMap = dynamic_cast<const Graphs::ChoroplethMap*>(&graph);
        if (choroplethMap == nullptr)
            {
            return;
            }

        LoadGraphOptions(graph);

        if (m_kmlPicker != nullptr && !choroplethMap->GetKMLFilePath().empty())
            {
            m_kmlPicker->SetPath(choroplethMap->GetKMLFilePath());
            }
        // SetPath() does not fire the picker's changed event, so fill the dropdown here
        PopulateKeyFieldChoices(choroplethMap->GetKMLFilePath());
        m_kmlIdField = choroplethMap->GetKMLIdField();
        m_showLabels = choroplethMap->IsShowingRegionLabels();
        m_showGraticule = choroplethMap->IsShowingGraticule();
        m_labelDisplay = static_cast<int>(choroplethMap->GetLabelDisplay());
        m_noDataFillStyle = NoDataFillStyleToChoiceIndex(choroplethMap->GetNoDataFillStyle());
        m_classificationMethod = static_cast<int>(choroplethMap->GetClassificationMethod());
        m_classCount = static_cast<int>(choroplethMap->GetClassCount());

        // pick the source dataset, then restore the column selections
        if (m_datasetChoice != nullptr && !choroplethMap->GetDataSourceName().empty())
            {
            const int foundDataset =
                m_datasetChoice->FindString(choroplethMap->GetDataSourceName());
            if (foundDataset != wxNOT_FOUND)
                {
                m_datasetChoice->SetSelection(foundDataset);
                }
            }

        m_keyColumn = choroplethMap->GetDataSourceKeyColumn();
        if (choroplethMap->IsCategoricalShading())
            {
            m_categoryColumn = choroplethMap->GetValueColumnName();
            }
        else
            {
            m_valueColumn = choroplethMap->GetValueColumnName();
            }
        m_symbolColumn = choroplethMap->GetProportionalSymbolColumnName();
        m_symbolColor = choroplethMap->GetProportionalSymbolColor();
        if (m_symbolColorPicker != nullptr)
            {
            m_symbolColorPicker->SetColour(m_symbolColor);
            }
        UpdateVariableLabels();

        TransferDataToWindow();
        UpdateClassificationControls();
        }
    } // namespace Wisteria::UI
