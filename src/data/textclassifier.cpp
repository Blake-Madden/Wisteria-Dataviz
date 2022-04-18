#include "textclassifier.h"

namespace Wisteria::Data
    {
    //----------------------------------------------------------------
    void TextClassifier::SetClassifierData(std::shared_ptr<const Data::Dataset> classifierData,
        const wxString categoryColumnName,
        const wxString patternsColumnName)
        {
        // reset
        m_categoryPatternsMap.clear();
        m_categoryColumnName.clear();

        auto categoryCol = classifierData->GetCategoricalColumn(categoryColumnName);
        if (categoryCol == classifierData->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': category column not found for text classifier."), categoryColumnName));
            }
        auto patternCol = classifierData->GetCategoricalColumn(patternsColumnName);
        if (patternCol == classifierData->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': patterns column not found for text classifier."), patternsColumnName));
            }

        // used later when classifying a dataset
        m_categoryColumnName = categoryColumnName;
        m_categoriesStringTable = categoryCol->GetStringTable();

        // build a map of unique categories and all the regexes connected to them.
        for (size_t i = 0; i < classifierData->GetRowCount(); ++i)
            {
            // make sure the regex is OK before loading it for later
            wxRegEx re(patternCol->GetCategoryLabel(patternCol->GetValue(i)));
            if (re.IsValid())
                {
                m_categoryPatternsMap.insert(categoryCol->GetValue(i),
                    std::make_shared<wxRegEx>(patternCol->GetCategoryLabel(patternCol->GetValue(i))));
                }
            else
                {
                wxLogWarning("'%s': regular expression syntax error for category '%s.'",
                             patternCol->GetCategoryLabel(patternCol->GetValue(i)),
                             categoryCol->GetCategoryLabel(categoryCol->GetValue(i)));
                }
            }
        }

    //----------------------------------------------------------------
    std::pair<std::shared_ptr<Data::Dataset>, std::shared_ptr<Data::Dataset>>
        TextClassifier::ClassifyData(
                    std::shared_ptr<const Data::Dataset> contentData,
                    const wxString contentColumnName)
        {
        // nothing patterns or categories loaded from previous call to SetClassifierData()?
        if (m_categoryPatternsMap.get_data().size() == 0)
            { return std::make_pair(nullptr, nullptr); }

        auto contentColumn = contentData->GetCategoricalColumn(contentColumnName);
        if (contentColumn == contentData->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': content column not found in dataset being classified."), contentColumnName));
            }

        // output will be the comments and categories that they matched against
        auto classifiedData = std::make_shared<Data::Dataset>();
        classifiedData->AddCategoricalColumn(contentColumnName, contentColumn->GetStringTable());
        classifiedData->AddCategoricalColumn(m_categoryColumnName, m_categoriesStringTable);

        auto unclassifiedData = std::make_shared<Data::Dataset>();
        unclassifiedData->AddCategoricalColumn(contentColumnName, contentColumn->GetStringTable());

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
                    classifiedData->AddRow(Data::RowInfo().Categoricals(
                        { contentColumn->GetValue(i) , id }));
                    }
                }
            if (!matchedAnyCategory)
                {
                unclassifiedData->AddRow(Data::RowInfo().Categoricals(
                    { contentColumn->GetValue(i) }));
                }
            }

        return std::make_pair(classifiedData, unclassifiedData);
        }
    }