///////////////////////////////////////////////////////////////////////////////
// Name:        textclassifier.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "textclassifier.h"

namespace Wisteria::Data
    {
    //----------------------------------------------------------------
    void
    TextClassifier::SetClassifierData(const std::shared_ptr<const Data::Dataset>& classifierData,
                                      const wxString& categoryColumnName,
                                      const std::optional<wxString>& subCategoryColumnName,
                                      const wxString& patternsColumnName,
                                      const std::optional<wxString>& negationPatternsColumnName)
        {
        // reset
        m_categoryPatternsMap.clear();
        m_categoryColumnName.clear();
        m_subCategoryColumnName = std::nullopt;

        auto categoryCol = classifierData->GetCategoricalColumn(categoryColumnName);
        if (categoryCol == classifierData->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': category column not found for text classifier."),
                                 categoryColumnName)
                    .ToUTF8());
            }
        auto subCategoryCol =
            (subCategoryColumnName.has_value() ?
                 classifierData->GetCategoricalColumn(subCategoryColumnName.value()) :
                 classifierData->GetCategoricalColumns().cend());
        if (subCategoryColumnName &&
            subCategoryCol == classifierData->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': sub-category column not found for text classifier."),
                                 subCategoryColumnName.value())
                    .ToUTF8());
            }
        auto patternCol = classifierData->GetCategoricalColumn(patternsColumnName);
        if (patternCol == classifierData->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': patterns column not found for text classifier."),
                                 patternsColumnName)
                    .ToUTF8());
            }
        auto negationPatternCol =
            (negationPatternsColumnName.has_value() ?
                 classifierData->GetCategoricalColumn(negationPatternsColumnName.value()) :
                 classifierData->GetCategoricalColumns().cend());

        // used later when classifying a dataset
        m_categoryColumnName = categoryColumnName;
        m_subCategoryColumnName = subCategoryColumnName;
        m_categoriesStringTable = categoryCol->GetStringTable();
        m_subCategoriesStringTable =
            (subCategoryColumnName ? subCategoryCol->GetStringTable() :
                                     ColumnWithStringTable::StringTableType());
        auto subCatMDCode = ColumnWithStringTable::FindMissingDataCode(m_subCategoriesStringTable);
        // if no missing data value in the string table, then add it
        if (!subCatMDCode)
            {
            subCatMDCode = ColumnWithStringTable::GetNextKey(m_subCategoriesStringTable);
            m_subCategoriesStringTable.insert(std::make_pair(subCatMDCode.value(), wxEmptyString));
            }

        // build a map of unique categories and all the regexes connected to them.
        for (size_t i = 0; i < classifierData->GetRowCount(); ++i)
            {
            // make sure the regex is OK before loading it for later
            const wxString reValue = patternCol->GetLabelFromID(patternCol->GetValue(i));
            const wxRegEx reg(reValue);

            const wxString negatingReValue =
                (negationPatternCol != classifierData->GetCategoricalColumns().cend()) ?
                    negationPatternCol->GetLabelFromID(negationPatternCol->GetValue(i)) :
                    wxString();

            if (!reValue.empty() && reg.IsValid())
                {
                m_categoryPatternsMap.insert(
                    std::make_pair(categoryCol->GetValue(i),
                                   (subCategoryColumnName ? subCategoryCol->GetValue(i) :
                                                            subCatMDCode.value())),
                    std::make_pair(std::make_shared<wxRegEx>(reValue),
                                   // empty string can be seen as valid, so an uninitialized wxRegEx
                                   // if there is no string
                                   !negatingReValue.empty() ?
                                       std::make_shared<wxRegEx>(negatingReValue) :
                                       std::make_shared<wxRegEx>()));
                }
            else
                {
                wxLogWarning(L"'%s': regular expression syntax error for category '%s.'", reValue,
                             categoryCol->GetLabelFromID(categoryCol->GetValue(i)));
                }
            }
        }

    //----------------------------------------------------------------
    std::pair<std::shared_ptr<Data::Dataset>, std::shared_ptr<Data::Dataset>>
    TextClassifier::ClassifyData(const std::shared_ptr<const Data::Dataset>& contentData,
                                 const wxString& contentColumnName) const
        {
        // nothing patterns or categories loaded from previous call to SetClassifierData()?
        if (m_categoryPatternsMap.get_data().empty())
            {
            return std::make_pair(nullptr, nullptr);
            }

        auto contentColumn = contentData->GetCategoricalColumn(contentColumnName);
        if (contentColumn == contentData->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': content column not found in dataset being classified."),
                                 contentColumnName)
                    .ToUTF8());
            }

        // output will be the comments and categories that they matched against
        auto classifiedData = std::make_shared<Data::Dataset>();
        classifiedData->AddCategoricalColumn(contentColumnName, contentColumn->GetStringTable());
        classifiedData->AddCategoricalColumn(m_categoryColumnName, m_categoriesStringTable);
        if (m_subCategoryColumnName)
            {
            classifiedData->AddCategoricalColumn(m_subCategoryColumnName.value(),
                                                 m_subCategoriesStringTable);
            }

        auto unclassifiedData = std::make_shared<Data::Dataset>();
        unclassifiedData->AddCategoricalColumn(contentColumnName, contentColumn->GetStringTable());
        const auto mdCode =
            ColumnWithStringTable::FindMissingDataCode(contentColumn->GetStringTable());

        for (size_t i = 0; i < contentData->GetRowCount(); ++i)
            {
            bool matchedAnyCategory{ false };
            // compare each comment from the data against each category...
            for (const auto& [id, regexes] : m_categoryPatternsMap.get_data())
                {
                // ...by comparing it against each regex in every category
                bool categoryRegexMatched{ false };
                for (const auto& reg : regexes.first)
                    {
                    if (reg.first->IsValid() &&
                        reg.first->Matches(
                            contentColumn->GetLabelFromID(contentColumn->GetValue(i))) &&
                        // either no negating regex or it doesn't match it
                        (!reg.second->IsValid() ||
                         !reg.second->Matches(
                             contentColumn->GetLabelFromID(contentColumn->GetValue(i)))))
                        {
                        categoryRegexMatched = true;
                        matchedAnyCategory = true;
                        break;
                        }
                    }
                // if any regex from the category matched the comment,
                // then add a row to the output containing the comment and the
                // category ID next to it
                if (categoryRegexMatched)
                    {
                    if (m_subCategoryColumnName)
                        {
                        classifiedData->AddRow(Data::RowInfo().Categoricals(
                            { contentColumn->GetValue(i), id.first, id.second }));
                        }
                    else
                        {
                        classifiedData->AddRow(
                            Data::RowInfo().Categoricals({ contentColumn->GetValue(i), id.first }));
                        }
                    }
                }
            if (!matchedAnyCategory)
                {
                // don't write out empty comments
                if (contentColumn->GetValue(i) != mdCode)
                    {
                    unclassifiedData->AddRow(
                        Data::RowInfo().Categoricals({ contentColumn->GetValue(i) }));
                    }
                }
            }

        return std::make_pair(classifiedData, unclassifiedData);
        }
    } // namespace Wisteria::Data
