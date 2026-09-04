///////////////////////////////////////////////////////////////////////////////
// Name:        wisteriaview_edit_data.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "../base/pdfreportprintout.h"
#include "../base/reportprintout.h"
#include "../base/svgreportprintout.h"
#include "../ui/controls/datasetgridtable.h"
#include "../ui/dialogs/datasetimportdlg.h"
#include "../ui/dialogs/editors/insertboxplotdlg.h"
#include "../ui/dialogs/editors/insertbubbleplotdlg.h"
#include "../ui/dialogs/editors/insertcandlestickplotdlg.h"
#include "../ui/dialogs/editors/insertcatbarchartdlg.h"
#include "../ui/dialogs/editors/insertchernoffdlg.h"
#include "../ui/dialogs/editors/insertcommonaxisdlg.h"
#include "../ui/dialogs/editors/insertganttchartdlg.h"
#include "../ui/dialogs/editors/insertheatmapdlg.h"
#include "../ui/dialogs/editors/inserthistogramdlg.h"
#include "../ui/dialogs/editors/insertimgdlg.h"
#include "../ui/dialogs/editors/insertitemdlg.h"
#include "../ui/dialogs/editors/insertlabeldlg.h"
#include "../ui/dialogs/editors/insertlikertdlg.h"
#include "../ui/dialogs/editors/insertlineplotdlg.h"
#include "../ui/dialogs/editors/insertlrroadmapdlg.h"
#include "../ui/dialogs/editors/insertmultiserieslineplotdlg.h"
#include "../ui/dialogs/editors/insertpagedlg.h"
#include "../ui/dialogs/editors/insertpiechartdlg.h"
#include "../ui/dialogs/editors/insertproconroadmapdlg.h"
#include "../ui/dialogs/editors/insertracetrackchartdlg.h"
#include "../ui/dialogs/editors/insertsankeydiagramdlg.h"
#include "../ui/dialogs/editors/insertscalechartdlg.h"
#include "../ui/dialogs/editors/insertscatterplotdlg.h"
#include "../ui/dialogs/editors/insertshapedlg.h"
#include "../ui/dialogs/editors/insertstemandleafdlg.h"
#include "../ui/dialogs/editors/inserttabledlg.h"
#include "../ui/dialogs/editors/insertwafflechartdlg.h"
#include "../ui/dialogs/editors/insertwcurvedlg.h"
#include "../ui/dialogs/editors/insertwilmarthbridgeplotdlg.h"
#include "../ui/dialogs/editors/insertwlsparklinedlg.h"
#include "../ui/dialogs/editors/insertwordclouddlg.h"
#include "../ui/dialogs/editors/joindlg.h"
#include "../ui/dialogs/editors/pivotlongerdlg.h"
#include "../ui/dialogs/editors/pivotwiderrdlg.h"
#include "../ui/dialogs/editors/subsetdlg.h"
#include "../ui/dialogs/pdfexportdlg.h"
#include "../ui/dialogs/projectsettingsdlg.h"
#include "../ui/dialogs/svgexportdlg.h"
#include "wisteriaapp.h"
#include "wisteriadoc.h"
#include "wisteriaview.h"
#include <array>
#include <wx/rearrangectrl.h>

//-------------------------------------------
void WisteriaView::PopulateConstantsGrid()
    {
    if (m_constantsGrid == nullptr)
        {
        return;
        }

    // clear existing rows
    if (m_constantsGrid->GetNumberRows() > 0)
        {
        m_constantsGrid->DeleteRows(0, m_constantsGrid->GetNumberRows());
        }

    // build the dataset name choices for the dropdown
    // (empty string = top-level constant, otherwise a dataset name)
    wxArrayString dsChoices = { wxString{} };
    for (const auto& [dsName, dataset] : m_reportBuilder.GetDatasets())
        {
        if (!dsName.empty())
            {
            dsChoices.Add(dsName);
            }
        }

    int row{ 0 };

    // helper to set the Dataset cell's choice editor for a row
    const auto setDatasetChoiceEditor = [this, &dsChoices](int gridRow)
    { m_constantsGrid->SetCellEditor(gridRow, 0, new wxGridCellChoiceEditor(dsChoices)); };

    // add top-level constants (no dataset)
    for (const auto& c : m_reportBuilder.GetConstants())
        {
        m_constantsGrid->AppendRows(1);
        m_constantsGrid->SetCellValue(row, 0, wxString{});
        m_constantsGrid->SetCellValue(row, 1, c.m_name);
        m_constantsGrid->SetCellValue(row, 2, c.m_value);
        m_constantsGrid->SetCellValue(row, 3, m_reportBuilder.GetExpandedValue(c.m_name));
        setDatasetChoiceEditor(row);
        ++row;
        }

    // add dataset formula constants
    for (const auto& [dsName, txOpts] : m_reportBuilder.GetDatasetTransformOptions())
        {
        for (const auto& f : txOpts.m_formulas)
            {
            m_constantsGrid->AppendRows(1);
            m_constantsGrid->SetCellValue(row, 0, dsName);
            m_constantsGrid->SetCellValue(row, 1, f.m_name);
            m_constantsGrid->SetCellValue(row, 2, f.m_value);
            m_constantsGrid->SetCellValue(row, 3, m_reportBuilder.GetExpandedValue(f.m_name));
            setDatasetChoiceEditor(row);
            ++row;
            }
        }

    m_constantsGrid->AutoSizeColumns(false);
    // ensure columns are at least wide enough for their header labels plus icon
    const int minColWidth = m_constantsGrid->FromDIP(120);
    for (int col = 0; col < m_constantsGrid->GetNumberCols(); ++col)
        {
        if (m_constantsGrid->GetColSize(col) < minColWidth)
            {
            m_constantsGrid->SetColSize(col, minColWidth);
            }
        }
    }

//-------------------------------------------
void WisteriaView::OnConstantEdited(wxGridEvent& event)
    {
    const int row = event.GetRow();
    const int col = event.GetCol();

    // the new dataset value (after editing)
    const wxString newDsName = m_constantsGrid->GetCellValue(row, 0);
    const wxString name = m_constantsGrid->GetCellValue(row, 1);
    const wxString value = m_constantsGrid->GetCellValue(row, 2);

    // if dataset column was edited...
    if (col == 0)
        {
        // dataset column changed: move the constant between
        // top-level and a dataset's formulas (or between datasets)
        const wxString oldDsName = event.GetString();

        // remove from old location
        if (oldDsName.empty())
            {
            // was a top-level constant
            auto& constants = m_reportBuilder.GetConstants();
            constants.erase({ name, value });
            // force a reset of the calculated values mapped to the constants
            m_reportBuilder.SetConstants(constants);
            }
        else
            {
            // was a dataset formula
            auto& txOpts = m_reportBuilder.GetDatasetTransformOptions();
            auto txIt = txOpts.find(oldDsName);
            if (txIt != txOpts.end())
                {
                auto& formulas = txIt->second.m_formulas;
                formulas.erase(std::ranges::remove_if(
                                   formulas,
                                   [&name](const Wisteria::ReportBuilder::DatasetFormulaInfo& f)
                                   { return f.m_name == name; })
                                   .begin(),
                               formulas.end());
                m_reportBuilder.SetDatasetTransformOptions(oldDsName, txIt->second);
                }
            }

        // add to new location
        if (newDsName.empty())
            {
            // moving to top-level constants
            auto& constants = m_reportBuilder.GetConstants();
            constants.emplace(name, value);
            // force a reset of the calculated values mapped to the constants
            m_reportBuilder.SetConstants(constants);
            }
        else
            {
            // moving to a dataset's formulas
            auto& allTxOpts = m_reportBuilder.GetDatasetTransformOptions();
            allTxOpts[newDsName].m_formulas.emplace_back(name, value);
            m_reportBuilder.SetDatasetTransformOptions(newDsName, allTxOpts[newDsName]);
            try
                {
                m_reportBuilder.RecalcFormula(name, value, newDsName);
                }
            catch (const std::exception& exc)
                {
                wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Formula Error"),
                             wxOK | wxICON_ERROR, m_frame);
                }
            }
        }
    // ...or other columns of a "regular" constant
    else if (newDsName.empty())
        {
        auto& constants = m_reportBuilder.GetConstants();
        // if name changed, delete the old constant and insert new one
        if (col == 1)
            {
            const wxString oldName{ event.GetString() };
            constants.erase({ oldName, value });
            constants.insert({ name, value });
            }
        // ...or value changed
        else
            {
            auto pos = constants.find({ name, value });
            auto nh = constants.extract(pos);
            nh.value().m_value = value;
            constants.insert(std::move(nh));
            }

        m_reportBuilder.SetConstants(constants);
        }
    // ...or other column of a dataset formula
    else
        {
        // editing Name or Value of a dataset formula
        auto& txOpts = m_reportBuilder.GetDatasetTransformOptions();
        auto txIt = txOpts.find(newDsName);
        if (txIt == txOpts.end())
            {
            return;
            }

        int formulaIdx{ 0 };
        for (int r = 0; r < row; ++r)
            {
            if (m_constantsGrid->GetCellValue(r, 0) == newDsName)
                {
                ++formulaIdx;
                }
            }

        if (static_cast<size_t>(formulaIdx) >= txIt->second.m_formulas.size())
            {
            return;
            }

        if (col == 1)
            {
            txIt->second.m_formulas[formulaIdx].m_name = name;
            }
        else if (col == 2)
            {
            txIt->second.m_formulas[formulaIdx].m_value = value;
            }

        m_reportBuilder.SetDatasetTransformOptions(newDsName, txIt->second);
        try
            {
            m_reportBuilder.RecalcFormula(name, value, newDsName);
            }
        catch (const std::exception& exc)
            {
            wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Formula Error"), wxOK | wxICON_ERROR,
                         m_frame);
            }
        }

    PopulateConstantsGrid();
    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnAddConstant([[maybe_unused]] wxCommandEvent& event)
    {
    const int newRow = m_constantsGrid->GetNumberRows();
    m_constantsGrid->AppendRows(1);

    // build the dataset name choices for the dropdown
    wxArrayString dsChoices;
    for (const auto& [dsName, dataset] : m_reportBuilder.GetDatasets())
        {
        if (!dsName.empty())
            {
            dsChoices.Add(dsName);
            }
        }
    m_constantsGrid->SetCellEditor(newRow, 0, new wxGridCellChoiceEditor(dsChoices));

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnDeleteConstant([[maybe_unused]] wxCommandEvent& event)
    {
    std::set<int> selectedRows;

    // rows where the entire row was selected (via labels)
    wxArrayInt fullRows = m_constantsGrid->GetSelectedRows();
    for (int row : fullRows)
        {
        selectedRows.insert(row);
        }

    // rows from rectangular block selections
    wxGridBlocks blocks = m_constantsGrid->GetSelectedBlocks();
    for (const auto& block : blocks)
        {
        for (int r = block.GetTopRow(); r <= block.GetBottomRow(); ++r)
            {
            selectedRows.insert(r);
            }
        }

    // rows from individually selected cells
    wxGridCellCoordsArray cells = m_constantsGrid->GetSelectedCells();
    for (const auto& cell : cells)
        {
        selectedRows.insert(cell.GetRow());
        }

    if (selectedRows.empty())
        {
        wxMessageBox(_(L"Please select a constant to delete."), _(L"Delete Constants"), wxOK,
                     m_frame);
        return;
        }

    if (wxMessageBox(wxPLURAL(L"Delete selected constant?", L"Delete selected constants?",
                              selectedRows.size()),
                     _(L"Delete Constants"), wxYES_NO | wxICON_QUESTION, m_frame) != wxYES)
        {
        return;
        }

    for (auto row : selectedRows)
        {
        const auto constantName = m_constantsGrid->GetCellValue(row, 1);
        if (m_constantsGrid->GetCellValue(row, 0).empty())
            {
            m_reportBuilder.GetConstants().erase({ constantName, wxString{} });
            }
        else
            {
            const wxString dsName{ m_constantsGrid->GetCellValue(row, 0) };
            auto& txOpts = m_reportBuilder.GetDatasetTransformOptions();
            auto txIt = txOpts.find(dsName);
            if (txIt == txOpts.end())
                {
                return;
                }
            auto nh = txOpts.extract(txIt);
            std::erase(nh.mapped().m_formulas,
                       Wisteria::ReportBuilder::DatasetFormulaInfo{ constantName, wxString{} });
            txOpts.insert(std::move(nh));
            }
        }
    PopulateConstantsGrid();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnRibbonAddConstant([[maybe_unused]] wxCommandEvent& event)
    {
    // select the Constants folder (index 1) in the sidebar
    m_sideBar->SelectFolder(1, true);
    // trigger the sidebar click so the constants grid is shown
    wxCommandEvent sidebarEvt(Wisteria::UI::wxEVT_SIDEBAR_CLICK);
    sidebarEvt.SetInt(m_constantsGrid->GetId());
    OnSidebarClick(sidebarEvt);
    // add a new constant
    wxCommandEvent addEvt(wxEVT_MENU, wxID_ADD);
    OnAddConstant(addEvt);
    }

//-------------------------------------------
void WisteriaView::OnEditDataset([[maybe_unused]] wxCommandEvent& event)
    {
    // get the selected dataset
    const auto [parentFolder, subItem] = m_sideBar->GetSelectedSubItemId();
    if (m_sideBar->GetSelectedFolder() != 0 || !subItem)
        {
        wxMessageBox(_(L"Please select a dataset."), _(L"Dataset Selection"), wxOK | wxICON_WARNING,
                     m_frame);
        return;
        }

    const auto selectedDatasetName = m_sideBar->GetSelectedLabel();
    const auto foundDs = GetReportBuilder().GetDatasets().find(selectedDatasetName);
    if (foundDs == GetReportBuilder().GetDatasets().cend())
        {
        wxFAIL_MSG(L"Didn't find dataset when editing?!");
        return;
        }
    // check if this is an imported dataset
    const auto foundDsImportOptions =
        GetReportBuilder().GetDatasetImportOptions().find(selectedDatasetName);
    // check if this is a pivoted dataset
    const auto foundPivotOptions =
        GetReportBuilder().GetDatasetPivotOptions().find(selectedDatasetName);

    if (foundDsImportOptions != GetReportBuilder().GetDatasetImportOptions().cend())
        {
        // edit an imported dataset
        Wisteria::UI::DatasetImportDlg importDlg(m_frame, foundDsImportOptions->second.m_filePath,
                                                 foundDsImportOptions->second.m_importInfo,
                                                 foundDsImportOptions->second.m_columnPreviewInfo,
                                                 foundDsImportOptions->second.m_worksheet, wxID_ANY,
                                                 _(L"Edit Import Options"));
        if (importDlg.ShowModal() != wxID_OK)
            {
            return;
            }

        try
            {
            const auto& fullColumnPreview = importDlg.GetFullColumnPreviewInfo();
            const auto importInfo = importDlg.GetImportInfo();
            const auto worksheet = importDlg.GetWorksheet();
            // the user may have browsed to a different file from within the import dialog
            const auto filePath = importDlg.GetFilePath();

            // re-import the dataset with the new settings
            auto dataset = std::make_shared<Wisteria::Data::Dataset>();
            dataset->Import(filePath, importInfo, worksheet);
            dataset->SetName(selectedDatasetName.ToStdWstring());

            // update the report builder's stored dataset and import options
            m_reportBuilder.GetDatasets()[selectedDatasetName] = dataset;
            m_reportBuilder.GetDatasetImportOptions()[selectedDatasetName] = {
                filePath, foundDsImportOptions->second.m_importer, worksheet, fullColumnPreview,
                importInfo
            };
            m_reportBuilder.GetDatasetTransformOptions()[selectedDatasetName].m_columnNamesSort =
                importInfo.GetColumnNamesSort();

            ReloadProject();
            }
        catch (const std::exception& exc)
            {
            wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Import Error"), wxOK | wxICON_ERROR,
                         m_frame);
            }
        }
    else if (const auto foundSubsetOptions =
                 GetReportBuilder().GetDatasetSubsetOptions().find(selectedDatasetName);
             foundSubsetOptions != GetReportBuilder().GetDatasetSubsetOptions().cend())
        {
        // edit a subsetted dataset
        const auto& storedOpts = foundSubsetOptions->second;

        Wisteria::UI::SubsetOptions subsetOpts;
        subsetOpts.m_sourceDatasetName = storedOpts.m_sourceDatasetName;
        subsetOpts.m_outputName = selectedDatasetName;
        subsetOpts.m_filterType =
            static_cast<Wisteria::UI::SubsetOptions::FilterType>(storedOpts.m_filterType);
        for (const auto& filt : storedOpts.m_filters)
            {
            subsetOpts.m_filters.push_back({ filt.m_column, filt.m_operator, filt.m_values });
            }
        subsetOpts.m_sectionColumn = storedOpts.m_sectionColumn;
        subsetOpts.m_sectionStartLabel = storedOpts.m_sectionStartLabel;
        subsetOpts.m_sectionEndLabel = storedOpts.m_sectionEndLabel;
        subsetOpts.m_sectionIncludeSentinelLabels = storedOpts.m_sectionIncludeSentinelLabels;

        Wisteria::UI::SubsetDlg dlg(&m_reportBuilder, subsetOpts, m_frame);
        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto subsettedDataset = dlg.GetSubsettedDataset();
        if (subsettedDataset == nullptr)
            {
            return;
            }

        const auto outputName = dlg.GetOutputName();
        const auto dlgOpts = dlg.GetSubsetOptions();

        // update stored subset options from dialog results
        auto& updatedOpts = m_reportBuilder.GetDatasetSubsetOptions()[outputName];
        updatedOpts.m_sourceDatasetName = dlgOpts.m_sourceDatasetName;
        updatedOpts.m_filterType =
            static_cast<Wisteria::ReportBuilder::DatasetSubsetOptions::FilterType>(
                dlgOpts.m_filterType);
        updatedOpts.m_filters.clear();
        for (const auto& criterion : dlgOpts.m_filters)
            {
            updatedOpts.m_filters.push_back(
                { criterion.m_column, criterion.m_operator, criterion.m_values });
            }
        updatedOpts.m_sectionColumn = dlgOpts.m_sectionColumn;
        updatedOpts.m_sectionStartLabel = dlgOpts.m_sectionStartLabel;
        updatedOpts.m_sectionEndLabel = dlgOpts.m_sectionEndLabel;
        updatedOpts.m_sectionIncludeSentinelLabels = dlgOpts.m_sectionIncludeSentinelLabels;

        // update stored dataset
        subsettedDataset->SetName(outputName.ToStdWstring());
        m_reportBuilder.GetDatasets()[outputName] = subsettedDataset;

        ReloadProject();
        }
    else if (foundPivotOptions != GetReportBuilder().GetDatasetPivotOptions().cend())
        {
        // edit a pivoted dataset
        const auto& storedOpts = foundPivotOptions->second;
        std::shared_ptr<Wisteria::Data::Dataset> pivotedDataset;
        wxString outputName;

        if (storedOpts.m_type == Wisteria::ReportBuilder::PivotType::Wider)
            {
            const Wisteria::UI::PivotWiderOptions widerOpts{
                storedOpts.m_sourceDatasetName, selectedDatasetName,
                storedOpts.m_idColumns,         storedOpts.m_namesFromColumn,
                storedOpts.m_valuesFromColumns, storedOpts.m_namesSep,
                storedOpts.m_namesPrefix,       storedOpts.m_fillValue
            };

            Wisteria::UI::PivotWiderDlg dlg(&m_reportBuilder, widerOpts, m_frame);
            if (dlg.ShowModal() != wxID_OK)
                {
                return;
                }

            pivotedDataset = dlg.GetPivotedDataset();
            outputName = dlg.GetOutputName();

            if (pivotedDataset == nullptr)
                {
                return;
                }

            // update stored pivot options from dialog results
            const auto dlgOpts = dlg.GetPivotOptions();
            auto& pivotOpts = m_reportBuilder.GetDatasetPivotOptions()[outputName];
            pivotOpts.m_type = Wisteria::ReportBuilder::PivotType::Wider;
            pivotOpts.m_sourceDatasetName = dlgOpts.m_sourceDatasetName;
            pivotOpts.m_idColumns = dlgOpts.m_idColumns;
            pivotOpts.m_namesFromColumn = dlgOpts.m_namesFromColumn;
            pivotOpts.m_valuesFromColumns = dlgOpts.m_valuesFromColumns;
            pivotOpts.m_namesSep = dlgOpts.m_namesSep;
            pivotOpts.m_namesPrefix = dlgOpts.m_namesPrefix;
            pivotOpts.m_fillValue = dlgOpts.m_fillValue;
            }
        else
            {
            const Wisteria::UI::PivotLongerOptions longerOpts{
                storedOpts.m_sourceDatasetName, selectedDatasetName,  storedOpts.m_columnsToKeep,
                storedOpts.m_fromColumns,       storedOpts.m_namesTo, storedOpts.m_valuesTo,
                storedOpts.m_namesPattern
            };

            Wisteria::UI::PivotLongerDlg dlg(&m_reportBuilder, longerOpts, m_frame);
            if (dlg.ShowModal() != wxID_OK)
                {
                return;
                }

            pivotedDataset = dlg.GetPivotedDataset();
            outputName = dlg.GetOutputName();

            if (pivotedDataset == nullptr)
                {
                return;
                }

            // update stored pivot options from dialog results
            const auto dlgOpts = dlg.GetPivotOptions();
            auto& pivotOpts = m_reportBuilder.GetDatasetPivotOptions()[outputName];
            pivotOpts.m_type = Wisteria::ReportBuilder::PivotType::Longer;
            pivotOpts.m_sourceDatasetName = dlgOpts.m_sourceDatasetName;
            pivotOpts.m_columnsToKeep = dlgOpts.m_columnsToKeep;
            pivotOpts.m_fromColumns = dlgOpts.m_fromColumns;
            pivotOpts.m_namesTo = dlgOpts.m_namesTo;
            pivotOpts.m_valuesTo = dlgOpts.m_valuesTo;
            pivotOpts.m_namesPattern = dlgOpts.m_namesPattern;
            }

        // update stored dataset
        pivotedDataset->SetName(outputName.ToStdWstring());
        m_reportBuilder.GetDatasets()[outputName] = pivotedDataset;

        ReloadProject();
        }
    else if (const auto foundMergeOptions =
                 GetReportBuilder().GetDatasetMergeOptions().find(selectedDatasetName);
             foundMergeOptions != GetReportBuilder().GetDatasetMergeOptions().cend())
        {
        // edit a joined dataset
        const auto& storedOpts = foundMergeOptions->second;

        Wisteria::UI::JoinOptions joinOpts;
        joinOpts.m_sourceDatasetName = storedOpts.m_sourceDatasetName;
        joinOpts.m_otherDatasetName = storedOpts.m_otherDatasetName;
        joinOpts.m_outputName = selectedDatasetName;
        joinOpts.m_byColumns = storedOpts.m_byColumns;
        joinOpts.m_suffix = storedOpts.m_suffix;
        if (storedOpts.m_type == L"left-join-unique-first")
            {
            joinOpts.m_type = Wisteria::UI::JoinOptions::JoinType::LeftJoinUniqueFirst;
            }
        else if (storedOpts.m_type == L"left-join")
            {
            joinOpts.m_type = Wisteria::UI::JoinOptions::JoinType::LeftJoin;
            }
        else if (storedOpts.m_type == L"inner-join")
            {
            joinOpts.m_type = Wisteria::UI::JoinOptions::JoinType::InnerJoin;
            }
        else
            {
            joinOpts.m_type = Wisteria::UI::JoinOptions::JoinType::LeftJoinUniqueLast;
            }

        Wisteria::UI::JoinDlg dlg(&m_reportBuilder, joinOpts, m_frame);
        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }

        const auto joinedDataset = dlg.GetJoinedDataset();
        if (joinedDataset == nullptr)
            {
            return;
            }

        const auto outputName = dlg.GetOutputName();
        const auto dlgOpts = dlg.GetJoinOptions();

        // update stored merge options from dialog results
        auto& updatedOpts = m_reportBuilder.GetDatasetMergeOptions()[outputName];
        updatedOpts.m_sourceDatasetName = dlgOpts.m_sourceDatasetName;
        updatedOpts.m_otherDatasetName = dlgOpts.m_otherDatasetName;
        updatedOpts.m_byColumns = dlgOpts.m_byColumns;
        updatedOpts.m_suffix = dlgOpts.m_suffix;
        switch (dlgOpts.m_type)
            {
        case Wisteria::UI::JoinOptions::JoinType::LeftJoinUniqueLast:
            updatedOpts.m_type = L"left-join-unique-last";
            break;
        case Wisteria::UI::JoinOptions::JoinType::LeftJoinUniqueFirst:
            updatedOpts.m_type = L"left-join-unique-first";
            break;
        case Wisteria::UI::JoinOptions::JoinType::LeftJoin:
            updatedOpts.m_type = L"left-join";
            break;
        case Wisteria::UI::JoinOptions::JoinType::InnerJoin:
            updatedOpts.m_type = L"inner-join";
            break;
            }

        // update stored dataset
        joinedDataset->SetName(outputName.ToStdWstring());
        m_reportBuilder.GetDatasets()[outputName] = joinedDataset;

        ReloadProject();
        }
    }

//-------------------------------------------
void WisteriaView::OnDeleteDataset([[maybe_unused]] wxCommandEvent& event)
    {
    // verify a dataset is selected in the Data folder
    const auto [parentFolder, subItem] = m_sideBar->GetSelectedSubItemId();
    if (m_sideBar->GetSelectedFolder() != 0 || !subItem)
        {
        wxMessageBox(_(L"Please select a dataset."), _(L"Dataset Selection"), wxOK | wxICON_WARNING,
                     m_frame);
        return;
        }

    const auto selectedDatasetName = m_sideBar->GetSelectedLabel();
    if (!m_reportBuilder.GetDatasets().contains(selectedDatasetName))
        {
        wxFAIL_MSG(L"Didn't find dataset when deleting?!");
        return;
        }

    // collect dependent datasets (subsets, pivots, merges sourced from this dataset)
    std::vector<wxString> dependentNames;
    for (const auto& [name, opts] : m_reportBuilder.GetDatasetPivotOptions())
        {
        if (opts.m_sourceDatasetName.CmpNoCase(selectedDatasetName) == 0)
            {
            dependentNames.emplace_back(name);
            }
        }
    for (const auto& [name, opts] : m_reportBuilder.GetDatasetSubsetOptions())
        {
        if (opts.m_sourceDatasetName.CmpNoCase(selectedDatasetName) == 0)
            {
            dependentNames.emplace_back(name);
            }
        }
    for (const auto& [name, opts] : m_reportBuilder.GetDatasetMergeOptions())
        {
        if (opts.m_sourceDatasetName.CmpNoCase(selectedDatasetName) == 0)
            {
            dependentNames.emplace_back(name);
            }
        }

    // build confirmation message
    wxString confirmMsg = wxString::Format(
        _(L"Are you sure you want to delete the dataset \"%s\"?"), selectedDatasetName);
    if (!dependentNames.empty())
        {
        confirmMsg += _(L"\n\nThe following derived datasets will also be removed:\n");
        for (const auto& depName : dependentNames)
            {
            confirmMsg += L"\n    • " + depName;
            }
        }

    if (wxMessageBox(confirmMsg, _(L"Delete Dataset"), wxYES_NO | wxICON_QUESTION, m_frame) !=
        wxYES)
        {
        return;
        }

    wxWindowUpdateLocker wl{ m_frame };

    const auto dataFolderId = m_sideBar->GetFolder(0).GetId();

    // helper to remove a single dataset by name from all storage
    const auto removeDataset = [this, dataFolderId](const wxString& dsName)
    {
        // the sidebar subitem and grid share the same window ID,
        // so find the subitem by label and use its ID to locate the grid
        const auto [folderIdx, subIdx] = m_sideBar->FindSubItem(dsName);
        if (folderIdx.has_value() && subIdx.has_value())
            {
            const auto dsWindowId =
                m_sideBar->GetFolder(folderIdx.value()).GetSubItemId(subIdx.value());
            if (auto* window = m_workWindows.FindWindowById(dsWindowId); window != nullptr)
                {
                m_workWindows.RemoveWindowById(dsWindowId);
                window->Destroy();
                }
            m_sideBar->DeleteSubItemById(dataFolderId, dsWindowId);
            }

        // remove from all report builder maps
        m_reportBuilder.GetDatasets().erase(dsName);
        m_reportBuilder.GetDatasetImportOptions().erase(dsName);
        m_reportBuilder.GetDatasetPivotOptions().erase(dsName);
        m_reportBuilder.GetDatasetSubsetOptions().erase(dsName);
        m_reportBuilder.GetDatasetMergeOptions().erase(dsName);
        m_reportBuilder.GetDatasetTransformOptions().erase(dsName);
        std::erase(m_reportBuilder.GetDatasetInsertionOrder(), dsName);
    };

    // remove dependent datasets first
    for (const auto& depName : dependentNames)
        {
        removeDataset(depName);
        }

    // remove the selected dataset itself
    removeDataset(selectedDatasetName);

    // select the next dataset in the data folder, if any
    const auto& dataFolder = m_sideBar->GetFolder(0);
    if (dataFolder.GetSubItemCount() > 0)
        {
        m_sideBar->SelectSubItem(0, 0);
        }
    else
        {
        m_sideBar->SelectFolder(0, true, true);
        }

    m_workArea->Layout();
    m_sideBar->SaveState();
    m_sideBar->Refresh();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnInsertDataset([[maybe_unused]] wxCommandEvent& event)
    {
    wxFileDialog fileDlg(m_frame, _(L"Select Dataset"), wxString{}, wxString{},
                         Wisteria::Data::Dataset::GetDataFileFilter(),
                         wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);

    if (fileDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const wxString initialFilePath = fileDlg.GetPath();

    Wisteria::UI::DatasetImportDlg importDlg(m_frame, initialFilePath);
    if (importDlg.ShowModal() != wxID_OK)
        {
        return;
        }

    try
        {
        const auto columnPreview = importDlg.GetColumnPreviewInfo();
        const auto& fullColumnPreview = importDlg.GetFullColumnPreviewInfo();
        const auto importInfo = importDlg.GetImportInfo();
        const auto worksheet = importDlg.GetWorksheet();
        // the user may have browsed to a different file from within the import dialog
        const auto filePath = importDlg.GetFilePath();
        auto dataset = std::make_shared<Wisteria::Data::Dataset>();
        dataset->Import(filePath, importInfo, worksheet);

        wxString dsName =
            m_reportBuilder.GenerateUniqueDatasetName(wxFileName{ filePath }.GetName());
        if (m_reportBuilder.GetDatasets().contains(wxFileName{ filePath }.GetName()))
            {
            wxTextEntryDialog nameDlg(m_frame,
                                      _(L"A dataset with this name already exists.\n"
                                        "Please enter a different name:"),
                                      _(L"Dataset Name"), dsName);
            while (nameDlg.ShowModal() == wxID_OK)
                {
                dsName = nameDlg.GetValue().Strip(wxString::both);
                if (dsName.empty())
                    {
                    wxMessageBox(_(L"The name cannot be empty."), _(L"Name Required"),
                                 wxOK | wxICON_WARNING, m_frame);
                    continue;
                    }
                if (m_reportBuilder.GetDatasets().contains(dsName))
                    {
                    wxMessageBox(_(L"A dataset with this name already exists."),
                                 _(L"Duplicate Name"), wxOK | wxICON_WARNING, m_frame);
                    continue;
                    }
                break;
                }
            if (dsName.empty() || m_reportBuilder.GetDatasets().contains(dsName))
                {
                return;
                }
            }

        AddDatasetToProject(dataset, dsName, columnPreview,
                            { filePath, wxString{}, worksheet, fullColumnPreview, importInfo });
        m_reportBuilder.GetDatasetTransformOptions()[dsName].m_columnNamesSort =
            importInfo.GetColumnNamesSort();
        }
    catch (const std::exception& exc)
        {
        wxMessageBox(wxString::FromUTF8(exc.what()), _(L"Import Error"), wxOK | wxICON_ERROR,
                     m_frame);
        }
    }

//-------------------------------------------
void WisteriaView::OnPivotWider([[maybe_unused]] wxCommandEvent& event)
    {
    if (m_reportBuilder.GetDatasets().empty())
        {
        wxMessageBox(_(L"Please import a dataset first."), _(L"No Datasets"),
                     wxOK | wxICON_INFORMATION, m_frame);
        return;
        }

    Wisteria::UI::PivotWiderOptions widerOpts;
    widerOpts.m_mode = Wisteria::UI::PivotWiderOptions::Mode::Insert;
    if (IsDatasetSelected())
        {
        widerOpts.m_sourceDatasetName = m_sideBar->GetSelectedLabel();
        widerOpts.m_outputName =
            wxString::Format(_(L"%s (Pivoted Wider)"), m_sideBar->GetSelectedLabel());
        }

    Wisteria::UI::PivotWiderDlg dlg(&m_reportBuilder, widerOpts, m_frame);
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const auto pivotedDataset = dlg.GetPivotedDataset();
    if (pivotedDataset == nullptr)
        {
        return;
        }

    const auto outputName = dlg.GetOutputName();
    const auto dlgOpts = dlg.GetPivotOptions();
    Wisteria::ReportBuilder::DatasetPivotOptions pivotOpts;
    pivotOpts.m_type = Wisteria::ReportBuilder::PivotType::Wider;
    pivotOpts.m_sourceDatasetName = dlgOpts.m_sourceDatasetName;
    pivotOpts.m_idColumns = dlgOpts.m_idColumns;
    pivotOpts.m_namesFromColumn = dlgOpts.m_namesFromColumn;
    pivotOpts.m_valuesFromColumns = dlgOpts.m_valuesFromColumns;
    pivotOpts.m_namesSep = dlgOpts.m_namesSep;
    pivotOpts.m_namesPrefix = dlgOpts.m_namesPrefix;
    pivotOpts.m_fillValue = dlgOpts.m_fillValue;

    m_reportBuilder.SetDatasetPivotOptions(outputName, pivotOpts);
    AddDatasetToProject(pivotedDataset, outputName);

    // adjust the splitter sash to match the sidebar's new min width
    const auto minWidth = m_sideBar->GetMinSize().GetWidth();
    m_splitter->SetSashPosition(minWidth);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnPivotLonger([[maybe_unused]] wxCommandEvent& event)
    {
    if (m_reportBuilder.GetDatasets().empty())
        {
        wxMessageBox(_(L"Please import a dataset first."), _(L"No Datasets"),
                     wxOK | wxICON_INFORMATION, m_frame);
        return;
        }

    Wisteria::UI::PivotLongerOptions longerOpts;
    longerOpts.m_mode = Wisteria::UI::PivotLongerOptions::Mode::Insert;
    if (IsDatasetSelected())
        {
        longerOpts.m_sourceDatasetName = m_sideBar->GetSelectedLabel();
        longerOpts.m_outputName =
            wxString::Format(_(L"%s (Pivoted Longer)"), m_sideBar->GetSelectedLabel());
        }

    Wisteria::UI::PivotLongerDlg dlg(&m_reportBuilder, longerOpts, m_frame);
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const auto pivotedDataset = dlg.GetPivotedDataset();
    if (pivotedDataset == nullptr)
        {
        return;
        }

    const auto outputName = dlg.GetOutputName();
    const auto dlgOpts = dlg.GetPivotOptions();
    Wisteria::ReportBuilder::DatasetPivotOptions pivotOpts;
    pivotOpts.m_type = Wisteria::ReportBuilder::PivotType::Longer;
    pivotOpts.m_sourceDatasetName = dlgOpts.m_sourceDatasetName;
    pivotOpts.m_columnsToKeep = dlgOpts.m_columnsToKeep;
    pivotOpts.m_fromColumns = dlgOpts.m_fromColumns;
    pivotOpts.m_namesTo = dlgOpts.m_namesTo;
    pivotOpts.m_valuesTo = dlgOpts.m_valuesTo;
    pivotOpts.m_namesPattern = dlgOpts.m_namesPattern;

    m_reportBuilder.SetDatasetPivotOptions(outputName, pivotOpts);
    AddDatasetToProject(pivotedDataset, outputName);

    // adjust the splitter sash to match the sidebar's new min width
    const auto minWidth = m_sideBar->GetMinSize().GetWidth();
    m_splitter->SetSashPosition(minWidth);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnSubsetDataset([[maybe_unused]] wxCommandEvent& event)
    {
    if (m_reportBuilder.GetDatasets().empty())
        {
        wxMessageBox(_(L"Please import a dataset first."), _(L"No Datasets"),
                     wxOK | wxICON_INFORMATION, m_frame);
        return;
        }

    Wisteria::UI::SubsetOptions subOptions;
    subOptions.m_mode = Wisteria::UI::SubsetOptions::Mode::Insert;
    if (IsDatasetSelected())
        {
        subOptions.m_sourceDatasetName = m_sideBar->GetSelectedLabel();
        subOptions.m_outputName =
            wxString::Format(_(L"%s (Subset)"), m_sideBar->GetSelectedLabel());
        }

    Wisteria::UI::SubsetDlg dlg(&m_reportBuilder, subOptions, m_frame);

    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const auto subsettedDataset = dlg.GetSubsettedDataset();
    if (subsettedDataset == nullptr)
        {
        return;
        }

    const auto outputName = dlg.GetOutputName();
    const auto dlgOpts = dlg.GetSubsetOptions();

    Wisteria::ReportBuilder::DatasetSubsetOptions subsetOpts;
    subsetOpts.m_sourceDatasetName = dlgOpts.m_sourceDatasetName;
    subsetOpts.m_filterType =
        static_cast<Wisteria::ReportBuilder::DatasetSubsetOptions::FilterType>(
            dlgOpts.m_filterType);
    for (const auto& criterion : dlgOpts.m_filters)
        {
        subsetOpts.m_filters.push_back(
            { criterion.m_column, criterion.m_operator, criterion.m_values });
        }
    subsetOpts.m_sectionColumn = dlgOpts.m_sectionColumn;
    subsetOpts.m_sectionStartLabel = dlgOpts.m_sectionStartLabel;
    subsetOpts.m_sectionEndLabel = dlgOpts.m_sectionEndLabel;
    subsetOpts.m_sectionIncludeSentinelLabels = dlgOpts.m_sectionIncludeSentinelLabels;

    m_reportBuilder.SetDatasetSubsetOptions(outputName, subsetOpts);
    AddDatasetToProject(subsettedDataset, outputName);

    // adjust the splitter sash to match the sidebar's new min width
    const auto minWidth = m_sideBar->GetMinSize().GetWidth();
    m_splitter->SetSashPosition(minWidth);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::OnJoinDataset([[maybe_unused]] wxCommandEvent& event)
    {
    if (m_reportBuilder.GetDatasets().size() < 2)
        {
        wxMessageBox(_(L"Please import at least two datasets first."), _(L"Not Enough Datasets"),
                     wxOK | wxICON_INFORMATION, m_frame);
        return;
        }

    Wisteria::UI::JoinDlg dlg(&m_reportBuilder, m_frame);
    if (dlg.ShowModal() != wxID_OK)
        {
        return;
        }

    const auto joinedDataset = dlg.GetJoinedDataset();
    if (joinedDataset == nullptr)
        {
        return;
        }

    const auto outputName = dlg.GetOutputName();
    const auto dlgOpts = dlg.GetJoinOptions();

    Wisteria::ReportBuilder::DatasetMergeOptions mergeOpts;
    mergeOpts.m_sourceDatasetName = dlgOpts.m_sourceDatasetName;
    mergeOpts.m_otherDatasetName = dlgOpts.m_otherDatasetName;
    mergeOpts.m_byColumns = dlgOpts.m_byColumns;
    mergeOpts.m_suffix = dlgOpts.m_suffix;
    switch (dlgOpts.m_type)
        {
    case Wisteria::UI::JoinOptions::JoinType::LeftJoinUniqueLast:
        mergeOpts.m_type = L"left-join-unique-last";
        break;
    case Wisteria::UI::JoinOptions::JoinType::LeftJoinUniqueFirst:
        mergeOpts.m_type = L"left-join-unique-first";
        break;
    case Wisteria::UI::JoinOptions::JoinType::LeftJoin:
        mergeOpts.m_type = L"left-join";
        break;
    case Wisteria::UI::JoinOptions::JoinType::InnerJoin:
        mergeOpts.m_type = L"inner-join";
        break;
        }

    m_reportBuilder.SetDatasetMergeOptions(outputName, mergeOpts);
    AddDatasetToProject(joinedDataset, outputName);

    // adjust the splitter sash to match the sidebar's new min width
    const auto minWidth = m_sideBar->GetMinSize().GetWidth();
    m_splitter->SetSashPosition(minWidth);

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::AddDatasetToProject(const std::shared_ptr<Wisteria::Data::Dataset>& dataset,
                                       const wxString& name)
    {
    m_reportBuilder.GetDatasets().insert_or_assign(name, dataset);
    m_reportBuilder.GetDatasetInsertionOrder().push_back(name);

    const wxWindowID dsId = wxNewId();

    auto* table = new Wisteria::UI::DatasetGridTable(dataset);

    auto* grid = new wxGrid(m_workArea, dsId);
    grid->SetDoubleBuffered(true);
    grid->GetGridWindow()->SetDoubleBuffered(true);
    grid->SetTable(table, true);
    grid->SetDefaultCellFitMode(wxGridFitMode::Ellipsize());
    grid->EnableEditing(false);
    ApplyColumnHeaderIcons(grid, table);
    m_workArea->GetSizer()->Add(grid, wxSizerFlags{ 1 }.Expand());
    m_workArea->Layout();
    grid->AutoSizeColumns(false);
    AdjustGridColumnsForIcons(grid);
    grid->Hide();
    m_workWindows.AddWindow(grid);

    // add as subitem under the "Data" folder
    if (m_sideBar->GetFolderCount() > 0)
        {
        m_sideBar->InsertSubItemById(m_sideBar->GetFolder(0).GetId(), name, dsId,
                                     GetDatasetIconFromName(name));
        m_sideBar->SelectSubItemById(m_sideBar->GetFolder(0).GetId(), dsId);
        }

    m_workArea->Layout();
    m_sideBar->SaveState();
    m_sideBar->Refresh();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
void WisteriaView::AddDatasetToProject(
    const std::shared_ptr<Wisteria::Data::Dataset>& dataset, const wxString& name,
    const Wisteria::Data::Dataset::ColumnPreviewInfo& columnInfo,
    const Wisteria::ReportBuilder::DatasetImportOptions& importOptions)
    {
    m_reportBuilder.AddDataset(name, dataset, importOptions);

    const wxWindowID dsId = wxNewId();

    auto* table = columnInfo.empty() ? new Wisteria::UI::DatasetGridTable(dataset) :
                                       new Wisteria::UI::DatasetGridTable(dataset, columnInfo);

    // apply currency symbols from the column preview info
    size_t contIdx{ 0 };
    for (const auto& col : columnInfo)
        {
        if (col.m_type == Wisteria::Data::Dataset::ColumnImportType::Numeric)
            {
            if (!col.m_currencySymbol.empty())
                {
                table->SetCurrencySymbol(contIdx, col.m_currencySymbol);
                }
            ++contIdx;
            }
        }

    auto* grid = new wxGrid(m_workArea, dsId);
    grid->SetDoubleBuffered(true);
    grid->GetGridWindow()->SetDoubleBuffered(true);
    grid->SetTable(table, true);
    grid->SetDefaultCellFitMode(wxGridFitMode::Ellipsize());
    grid->EnableEditing(false);
    ApplyColumnHeaderIcons(grid, table);
    m_workArea->GetSizer()->Add(grid, wxSizerFlags{ 1 }.Expand());
    m_workArea->Layout();
    grid->AutoSizeColumns(false);
    AdjustGridColumnsForIcons(grid);
    grid->Hide();
    m_workWindows.AddWindow(grid);

    // add as subitem under the "Data" folder
    if (m_sideBar->GetFolderCount() > 0)
        {
        m_sideBar->InsertSubItemById(m_sideBar->GetFolder(0).GetId(), name, dsId,
                                     GetDatasetIconFromName(name));
        m_sideBar->SelectSubItemById(m_sideBar->GetFolder(0).GetId(), dsId);
        }

    m_workArea->Layout();
    m_sideBar->SaveState();
    m_sideBar->Refresh();

    GetDocument()->Modify(true);
    }

//-------------------------------------------
size_t WisteriaView::GetDatasetIconFromName(const wxString& name) const
    {
    if (const auto foundPos = GetReportBuilder().GetDatasetPivotOptions().find(name);
        foundPos != GetReportBuilder().GetDatasetPivotOptions().cend())
        {
        if (foundPos->second.m_type == Wisteria::ReportBuilder::PivotType::Wider)
            {
            return DATA_PIVOT_WIDER_ICON_INDEX;
            }
        else
            {
            return DATA_PIVOT_LONGER_ICON_INDEX;
            }
        }
    if (const auto foundPos = GetReportBuilder().GetDatasetSubsetOptions().find(name);
        foundPos != GetReportBuilder().GetDatasetSubsetOptions().cend())
        {
        return DATA_SUBSET_ICON_INDEX;
        }
    if (const auto foundPos = GetReportBuilder().GetDatasetMergeOptions().find(name);
        foundPos != GetReportBuilder().GetDatasetMergeOptions().cend())
        {
        return DATA_JOIN_ICON_INDEX;
        }
    return DATA_ICON_INDEX;
    }
