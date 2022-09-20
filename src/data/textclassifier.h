/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_TEXT_CLASSIFIER_H__
#define __WISTERIA_TEXT_CLASSIFIER_H__

#include "dataset.h"
#include "../util/frequencymap.h"

namespace Wisteria::Data
    {
    /** @brief Classifies the items from a text column into categories, based on a
          series of pre-defined categories and their respective regular expressions.

          Every text item is compared against each regex connected with a list of
          categories (and optional sub-categories); any matches will then associate
          the text with that category.
         
          This is useful for classifying free-form comments from a survey into categories,
          and then taking a frequency count of those categories.

         @par Usage:

          The first step is to load a dataset with the categories and the regular
          expressions used to classify text:
         
          | CATEGORY        | SUBCATEGORY | PATTERN                            | NEGATE_PATTERN |
          | :--             |             | :--                                |                |
          | Facilities      |             | (?i)stadium                        |                |
          | Facilities      | Parking Lot | (?i)\\bparking                     |                |
          | Athletics       |             | (?i)(foot\|basket\|base\|soft)ball |                |
          | Athletics       |             | (?i)stadium                        |                |
          | Food & Beverage |             | (?i)\\bfood\\b                     |                |
          | Food & Beverage |             | (?i)pretzel                        |                |
          | Programs        | CompSci     | (?i)software                       |                |     
          | Programs        | Engineering | (?i)engineer                       | (?i)software   |

          This will build a classifier with the categories `Facilities`, `Athletics`,
          `Food & Beverage`, and `Programs`. Each of these categories will have their own set of regexes.

          For example, any text that matches `(?i)stadium ` or `(?i)(basket|base|soft)ball`
          will be classified as `Athletics`. Likewise, that same text will additionally be classified
          as `Facilities` because that category also as the regex `(?i)stadium`.

          As another example, any text matching `(?i)\bparking` will be classified into
          `Facilities` along with the sub-category `Parking Lot`.

          Finally, matches can be negated with an optional negation column. In the above example,
          any comment that contains `(?i)engineer` will be classified into `Programs` and the sub-category
          `Engineering` **unless** it also contains `(?i)software`.

         @note The regex supported by this class is PCRE syntax described at
          https://www.pcre.org/current/doc/html/pcre2syntax.html.

          After loading a classifier dataset, you then load a dataset to analyze:

          | COMMENTS                                                           |
          | :--                                                                |
          | I love the football games. The stadium needs more seats though.    |
          | The parking lot is hard to find.                                   |
          | Wish they had hot pretzels at the softball games.                  |
          | The printer in the library was out of paper :(                     |
          | More classes for engineering (software development) would be nice. |
          | The chemical engineering classes are too tough.                    |

          This will result in two datasets; one with the comments categorized:

          | COMMENTS                                                           | CATEGORY        | SUBCATEGORY |
          | :--                                                                | :--             |             |
          | I love the football games. The stadium needs more seats though.    | Facilities      |             |
          | I love the football games. The stadium needs more seats though.    | Athletics       |             |
          | The parking lot is hard to find.                                   | Facilities      | Parking Lot |
          | Wish they had hot pretzels at the softball games.                  | Athletics       |             |
          | Wish they had hot pretzels at the softball games.                  | Food & Beverage |             |
          | More classes for engineering (software development) would be nice. | Programs        | CompSci     |
          | The chemical engineering classes are too tough.                    | Programs        | Engineering |

          And one with the uncategorized comments:

          | COMMENTS                                       |
          | :--                                            |
          | The printer in the library was out of paper :( |
         */
    class TextClassifier
        {
    public:
        /** @brief Sets the categories and their respective regular expressions
             used to classify text into them.
            @param classifierData The dataset with the categories and regexes in it.
            @param categoryColumnName The name of the column with the categories in it.\n
             Note that the same categories can be used throughout the file, with different
             regular expressions next to them. This allows for using numerous regexes to
             classify text into the same category.
            @param subCategoryColumnName The name of the column with sub-categories in it.
             (The category column next to this column represents the parent category for
             these values). If an entry in this column is empty, then strings classified
             by the parent regular expression will generically fall into the parent category
             (with no sub-category). Set this to @c std::nullopt to not use sub-categories.
            @param patternsColumnName The name of the column with the regular expression in it.
            @param negationPatternsColumnName The (optional) name of the column with a negating
             regular expression in it. This is used to negate any match found with the
             @c patternsColumnName column.
            @note Any invalid regular expressions loaded from the file will be logged using
             @c wxLogWarning().
            @throws std::runtime_error If the file can't be read or named columns aren't found,
             throws an exception.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.*/
        void SetClassifierData(std::shared_ptr<const Data::Dataset> classifierData,
                               const wxString& categoryColumnName,
                               const std::optional<wxString>& subCategoryColumnName,
                               const wxString& patternsColumnName,
                               const std::optional<wxString>& negationPatternsColumnName);
        /** @brief Classifies a column of text values into previously defined categories
             that rely on regular expression pattern matching.
            @param contentData The dataset with the text to be classified.
            @param contentColumnName The name of the text column that contains content to
             be classified. This would usually be a column of comments from a survey.
            @returns A pair of datasets.\n
             \n
             The first is a dataset filled with the comments and the categories that they matched against.
             Note that if a comment was classified into multiple categories,
             then a row will be added for each of these matches. In other words, the comment
             will appear multiple times in the output, with the different categories next to it.\n
             \n
             The second is a dataset of all comments that were not classified. This can be useful
             for retraining your classifier (i.e., adding more regexes to your classifier dataset).
            @throws std::runtime_error If the file can't be read or named columns aren't found,
             throws an exception.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.*/
        std::pair<std::shared_ptr<Data::Dataset>, std::shared_ptr<Data::Dataset>>
                        ClassifyData(
                            std::shared_ptr<const Data::Dataset> contentData,
                            const wxString& contentColumnName);
    private:
        using IdPair = std::pair<Data::GroupIdType, Data::GroupIdType>;
        // wxRegEx cannot be copy constructed by design, so use shared pointers instead
        using RegExPair = std::pair<std::shared_ptr<wxRegEx>, std::shared_ptr<wxRegEx>>;

        multi_value_aggregate_map<IdPair, RegExPair> m_categoryPatternsMap;
        wxString m_categoryColumnName;
        std::optional<wxString> m_subCategoryColumnName;
        ColumnWithStringTable::StringTableType m_categoriesStringTable;
        ColumnWithStringTable::StringTableType m_subCategoriesStringTable;
        };
    }

/** @}*/

#endif //__WISTERIA_TEXT_CLASSIFIER_H__
