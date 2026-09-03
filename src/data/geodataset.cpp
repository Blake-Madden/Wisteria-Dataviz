///////////////////////////////////////////////////////////////////////////////
// Name:        geodataset.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "geodataset.h"
#include <limits>
#include <map>
#include <set>

namespace Wisteria::Data
    {
    //---------------------------------------------------
    bool GeoDataset::ImportKML(const wxString& filePath, const GeoImportInfo& info)
        {
        KmlReader reader;
        if (!reader.LoadFile(filePath))
            {
            m_lastError = reader.GetLastError();
            return false;
            }
        return ImportRegions(reader, info);
        }

    //---------------------------------------------------
    bool GeoDataset::ImportRegionsFromText(const wxString& kmlText, const GeoImportInfo& info)
        {
        KmlReader reader;
        if (!reader.LoadText(kmlText))
            {
            m_lastError = reader.GetLastError();
            return false;
            }
        return ImportRegions(reader, info);
        }

    //---------------------------------------------------
    bool GeoDataset::ImportRegions(const KmlReader& reader, const GeoImportInfo& info)
        {
        m_lastError.clear();
        m_geometries.clear();
        m_boundingBox = GeoBoundingBox{};

        if (GetRowCount() > 0)
            {
            m_lastError = _(L"GeoDataset is already populated. Import into a new GeoDataset.");
            return false;
            }

        const auto& regions = reader.GetRegions();
        if (regions.empty())
            {
            m_lastError = reader.GetLastError().empty() ? _(L"No regions to import from KML.") :
                                                          reader.GetLastError();
            return false;
            }

        // gather the union of every placemark's attribute field names
        std::set<wxString> fieldNameSet;
        for (const auto& region : regions)
            {
            for (const auto& [fieldName, fieldValue] : region.m_attributes)
                {
                fieldNameSet.insert(fieldName);
                }
            }

        // split the fields into continuous and categorical, keeping a stable order
        std::vector<wxString> continuousFields;
        std::vector<wxString> categoricalFields;
        for (const auto& fieldName : fieldNameSet)
            {
            // the ID field is represented by the ID column, not a duplicate data column
            if (!info.GetIdField().empty() && info.GetIdField().CmpNoCase(fieldName) == 0)
                {
                continue;
                }
            if (IsContinuousField(fieldName, reader, info))
                {
                continuousFields.push_back(fieldName);
                }
            else
                {
                categoricalFields.push_back(fieldName);
                }
            }

        SetName(reader.GetName().empty() ? std::wstring{ _DT(L"KML Regions") } :
                                           reader.GetName().ToStdWstring());

        // create the columns up front, in the order the row values are written below
        if (info.IsImportingCentroids())
            {
            AddContinuousColumn(_DT(L"Centroid Longitude"));
            AddContinuousColumn(_DT(L"Centroid Latitude"));
            }
        for (const auto& fieldName : continuousFields)
            {
            AddContinuousColumn(fieldName);
            }

        // build a string table (and a value-to-code lookup) for each categorical field
        std::map<wxString, std::map<wxString, GroupIdType>> fieldValueCodes;
        for (const auto& fieldName : categoricalFields)
            {
            ColumnWithStringTable::StringTableType stringTable;
            stringTable.insert({ 0, wxString{} }); // code 0 is missing data
            GroupIdType nextKey{ 1 };
            auto& valueCodes = fieldValueCodes[fieldName];
            for (const auto& region : regions)
                {
                const wxString fieldValue = region.GetAttribute(fieldName);
                if (fieldValue.empty() || valueCodes.contains(fieldValue))
                    {
                    continue;
                    }
                valueCodes.emplace(fieldValue, nextKey);
                stringTable.insert({ nextKey, fieldValue });
                ++nextKey;
                }
            AddCategoricalColumn(fieldName, stringTable);
            }

        // write one row (and one parallel geometry entry) per region
        Reserve(regions.size());
        m_geometries.reserve(regions.size());
        for (const auto& region : regions)
            {
            const wxString idValue =
                info.GetIdField().empty() ? region.m_name : region.GetAttribute(info.GetIdField());

            std::vector<double> continuousValues;
            continuousValues.reserve((info.IsImportingCentroids() ? 2 : 0) +
                                     continuousFields.size());
            if (info.IsImportingCentroids())
                {
                const bool haveExtent = region.m_boundingBox.IsOk();
                const GeoCoordinate center =
                    haveExtent ? region.m_boundingBox.GetCenter() : GeoCoordinate{};
                continuousValues.push_back(haveExtent ? center.m_longitude :
                                                        std::numeric_limits<double>::quiet_NaN());
                continuousValues.push_back(haveExtent ? center.m_latitude :
                                                        std::numeric_limits<double>::quiet_NaN());
                }
            for (const auto& fieldName : continuousFields)
                {
                double parsedValue{ std::numeric_limits<double>::quiet_NaN() };
                const wxString fieldValue = region.GetAttribute(fieldName);
                if (!fieldValue.empty())
                    {
                    fieldValue.ToCDouble(&parsedValue);
                    }
                continuousValues.push_back(parsedValue);
                }

            std::vector<GroupIdType> categoricalValues;
            categoricalValues.reserve(categoricalFields.size());
            for (const auto& fieldName : categoricalFields)
                {
                const auto& valueCodes = fieldValueCodes[fieldName];
                const auto foundCode = valueCodes.find(region.GetAttribute(fieldName));
                categoricalValues.push_back(foundCode != valueCodes.cend() ? foundCode->second :
                                                                             GroupIdType{ 0 });
                }

            AddRow(
                RowInfo().Id(idValue).Continuous(continuousValues).Categoricals(categoricalValues));

            m_geometries.push_back(region);
            m_boundingBox.Encompass(region.m_boundingBox);
            }

        return true;
        }

    //---------------------------------------------------
    bool GeoDataset::IsContinuousField(const wxString& fieldName, const KmlReader& reader,
                                       const GeoImportInfo& info)
        {
        for (const auto& forcedField : info.GetContinuousFields())
            {
            if (forcedField.CmpNoCase(fieldName) == 0)
                {
                return true;
                }
            }
        for (const auto& forcedField : info.GetCategoricalFields())
            {
            if (forcedField.CmpNoCase(fieldName) == 0)
                {
                return false;
                }
            }

        // continuous only if every value present parses as a number
        bool sawAnyValue{ false };
        for (const auto& region : reader.GetRegions())
            {
            const wxString fieldValue = region.GetAttribute(fieldName);
            if (fieldValue.empty())
                {
                continue;
                }
            sawAnyValue = true;
            double parsedValue{ 0.0 };
            if (!fieldValue.ToCDouble(&parsedValue))
                {
                return false;
                }
            }
        return sawAnyValue;
        }

    //---------------------------------------------------
    std::optional<size_t> GeoDataset::FindRegionRow(const wxString& idValue) const
        {
        for (size_t row = 0; row < GetIdColumn().GetRowCount(); ++row)
            {
            if (GetIdColumn().GetValue(row) == idValue)
                {
                return row;
                }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    bool GeoDataset::CopyContinuousColumnFrom(const Dataset& source,
                                              const wxString& sourceKeyColumn,
                                              const wxString& sourceValueColumn,
                                              const wxString& targetColumnName)
        {
        m_lastError.clear();

        const auto sourceValueColumnIter = source.GetContinuousColumn(sourceValueColumn);
        if (sourceValueColumnIter == source.GetContinuousColumns().cend())
            {
            m_lastError = wxString::Format(
                _(L"'%s': continuous column not found in source dataset."), sourceValueColumn);
            return false;
            }

        const bool keyIsIdColumn = (source.GetIdColumn().GetName().CmpNoCase(sourceKeyColumn) == 0);
        auto sourceKeyColumnIter = source.GetCategoricalColumns().cend();
        if (!keyIsIdColumn)
            {
            sourceKeyColumnIter = source.GetCategoricalColumn(sourceKeyColumn);
            if (sourceKeyColumnIter == source.GetCategoricalColumns().cend())
                {
                m_lastError = wxString::Format(_(L"'%s': key column not found in source dataset."),
                                               sourceKeyColumn);
                return false;
                }
            }

        std::map<wxString, double> keyToValue;
        for (size_t row = 0; row < source.GetRowCount(); ++row)
            {
            const wxString keyValue =
                keyIsIdColumn ?
                    source.GetIdColumn().GetValue(row) :
                    sourceKeyColumnIter->GetLabelFromID(sourceKeyColumnIter->GetValue(row));
            keyToValue.insert_or_assign(keyValue, sourceValueColumnIter->GetValue(row));
            }

        const wxString newColumnName =
            targetColumnName.empty() ? sourceValueColumn : targetColumnName;
        AddContinuousColumn(newColumnName);
        const auto targetColumnIter = GetContinuousColumn(newColumnName);
        for (size_t row = 0; row < GetRowCount(); ++row)
            {
            const auto foundValue = keyToValue.find(GetIdColumn().GetValue(row));
            if (foundValue != keyToValue.cend())
                {
                targetColumnIter->SetValue(row, foundValue->second);
                }
            }

        return true;
        }

    //---------------------------------------------------
    bool GeoDataset::CopyCategoricalColumnFrom(const Dataset& source,
                                               const wxString& sourceKeyColumn,
                                               const wxString& sourceLabelColumn,
                                               const wxString& targetColumnName)
        {
        m_lastError.clear();

        const auto sourceLabelColumnIter = source.GetCategoricalColumn(sourceLabelColumn);
        if (sourceLabelColumnIter == source.GetCategoricalColumns().cend())
            {
            m_lastError = wxString::Format(
                _(L"'%s': categorical column not found in source dataset."), sourceLabelColumn);
            return false;
            }

        const bool keyIsIdColumn = (source.GetIdColumn().GetName().CmpNoCase(sourceKeyColumn) == 0);
        auto sourceKeyColumnIter = source.GetCategoricalColumns().cend();
        if (!keyIsIdColumn)
            {
            sourceKeyColumnIter = source.GetCategoricalColumn(sourceKeyColumn);
            if (sourceKeyColumnIter == source.GetCategoricalColumns().cend())
                {
                m_lastError = wxString::Format(_(L"'%s': key column not found in source dataset."),
                                               sourceKeyColumn);
                return false;
                }
            }

        std::map<wxString, wxString> keyToLabel;
        for (size_t row = 0; row < source.GetRowCount(); ++row)
            {
            const wxString keyValue =
                keyIsIdColumn ?
                    source.GetIdColumn().GetValue(row) :
                    sourceKeyColumnIter->GetLabelFromID(sourceKeyColumnIter->GetValue(row));
            keyToLabel.insert_or_assign(keyValue, sourceLabelColumnIter->GetLabelFromID(
                                                      sourceLabelColumnIter->GetValue(row)));
            }

        // Build the string table for the new column. Code 0 is missing data, then
        // one code per distinct label that a region actually matches.
        ColumnWithStringTable::StringTableType stringTable;
        stringTable.insert({ 0, wxString{} });
        std::map<wxString, GroupIdType> labelCodes;
        GroupIdType nextCode{ 1 };
        for (size_t row = 0; row < GetRowCount(); ++row)
            {
            const auto foundLabel = keyToLabel.find(GetIdColumn().GetValue(row));
            if (foundLabel == keyToLabel.cend() || foundLabel->second.empty() ||
                labelCodes.contains(foundLabel->second))
                {
                continue;
                }
            labelCodes.emplace(foundLabel->second, nextCode);
            stringTable.insert({ nextCode, foundLabel->second });
            ++nextCode;
            }

        const wxString newColumnName =
            targetColumnName.empty() ? sourceLabelColumn : targetColumnName;
        AddCategoricalColumn(newColumnName, stringTable);
        const auto targetColumnIter = GetCategoricalColumn(newColumnName);
        for (size_t row = 0; row < GetRowCount(); ++row)
            {
            const auto foundLabel = keyToLabel.find(GetIdColumn().GetValue(row));
            if (foundLabel == keyToLabel.cend() || foundLabel->second.empty())
                {
                continue;
                }
            const auto foundCode = labelCodes.find(foundLabel->second);
            if (foundCode != labelCodes.cend())
                {
                targetColumnIter->SetValue(row, foundCode->second);
                }
            }

        return true;
        }
    } // namespace Wisteria::Data
