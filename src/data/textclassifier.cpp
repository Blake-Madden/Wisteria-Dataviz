#include "textclassifier.h"

namespace Wisteria::Data
    {
    //----------------------------------------------------------------
    void TextClassifier::SetClassifierData(std::shared_ptr<const Data::Dataset> classifierData,
        const wxString& categoryColumnName,
        const std::optional<wxString>& subCategoryColumnName,
        const wxString& patternsColumnName)
        {
        // reset
        m_categoryPatternsMap.clear();
        m_categoryColumnName.clear();
        m_subCategoryColumnName = std::nullopt;

        auto categoryCol = classifierData->GetCategoricalColumn(categoryColumnName);
        if (categoryCol == classifierData->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': category column not found for text classifier."),
                categoryColumnName));
            }
        auto subCategoryCol = (subCategoryColumnName.has_value() ?
            classifierData->GetCategoricalColumn(subCategoryColumnName.value()) :
            classifierData->GetCategoricalColumns().cend());
        if (subCategoryColumnName &&
            subCategoryCol == classifierData->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': sub-category column not found for text classifier."),
                subCategoryColumnName.value()));
            }
        auto patternCol = classifierData->GetCategoricalColumn(patternsColumnName);
        if (patternCol == classifierData->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': patterns column not found for text classifier."),
                patternsColumnName));
            }

        // used later when classifying a dataset
        m_categoryColumnName = categoryColumnName;
        m_subCategoryColumnName = subCategoryColumnName;
        m_categoriesStringTable = categoryCol->GetStringTable();
        m_subCategoriesStringTable = (subCategoryColumnName ?
            subCategoryCol->GetStringTable() :
            ColumnWithStringTable::StringTableType());
        auto subCatMDCode = ColumnWithStringTable::FindMissingDataCode(m_subCategoriesStringTable);
        if (!subCatMDCode)
            { subCatMDCode = ColumnWithStringTable::GetNextKey(m_subCategoriesStringTable); }

        // build a map of unique categories and all the regexes connected to them.
        for (size_t i = 0; i < classifierData->GetRowCount(); ++i)
            {
            // make sure the regex is OK before loading it for later
            const auto reValue = patternCol->GetCategoryLabel(patternCol->GetValue(i));
            wxRegEx re(reValue);
            if (reValue.length() && re.IsValid())
                {
                m_categoryPatternsMap.insert(
                    std::make_pair(categoryCol->GetValue(i),
                        (subCategoryColumnName ? subCategoryCol->GetValue(i) : subCatMDCode.value())),
                    std::make_shared<wxRegEx>(patternCol->GetCategoryLabel(patternCol->GetValue(i))));
                }
            else
                {
                wxLogWarning(_(L"'%s': regular expression syntax error for category '%s.'"),
                             patternCol->GetCategoryLabel(patternCol->GetValue(i)),
                             categoryCol->GetCategoryLabel(categoryCol->GetValue(i)));
                }
            }
        }

    //----------------------------------------------------------------
    std::pair<std::shared_ptr<Data::Dataset>, std::shared_ptr<Data::Dataset>>
        TextClassifier::ClassifyData(
                    std::shared_ptr<const Data::Dataset> contentData,
                    const wxString& contentColumnName)
        {
        // nothing patterns or categories loaded from previous call to SetClassifierData()?
        if (m_categoryPatternsMap.get_data().size() == 0)
            { return std::make_pair(nullptr, nullptr); }

        auto contentColumn = contentData->GetCategoricalColumn(contentColumnName);
        if (contentColumn == contentData->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': content column not found in dataset being classified."),
                contentColumnName));
            }

        // output will be the comments and categories that they matched against
        auto classifiedData = std::make_shared<Data::Dataset>();
        classifiedData->AddCategoricalColumn(contentColumnName, contentColumn->GetStringTable());
        classifiedData->AddCategoricalColumn(m_categoryColumnName, m_categoriesStringTable);
        if (m_subCategoryColumnName)
            {
            classifiedData->AddCategoricalColumn(
                m_subCategoryColumnName.value(), m_subCategoriesStringTable);
            }

        auto unclassifiedData = std::make_shared<Data::Dataset>();
        unclassifiedData->AddCategoricalColumn(contentColumnName, contentColumn->GetStringTable());
        const auto mdCode = ColumnWithStringTable::FindMissingDataCode(contentColumn->GetStringTable());

        for (size_t i = 0; i < contentData->GetRowCount(); ++i)
            {
            bool matchedAnyCategory{ false };
            // compare each comment from the data against each catetory...
            for (const auto& [id, regexes] : m_categoryPatternsMap.get_data())
                {
                // ...by comparing it against each regex in every category
                bool categoryRegexMatched{ false };
                for (const auto& re : regexes.first)
                    {
                    if (re->IsValid() &&
                        re->Matches(contentColumn->GetCategoryLabel(contentColumn->GetValue(i))))
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
                        classifiedData->AddRow(Data::RowInfo().Categoricals(
                            { contentColumn->GetValue(i), id.first }));
                        }
                    }
                }
            if (!matchedAnyCategory)
                {
                // don't write out empty comments
                if (contentColumn->GetValue(i) != mdCode)
                    {
                    unclassifiedData->AddRow(Data::RowInfo().Categoricals(
                        { contentColumn->GetValue(i) }));
                    }
                }
            }

        return std::make_pair(classifiedData, unclassifiedData);
        }
    }