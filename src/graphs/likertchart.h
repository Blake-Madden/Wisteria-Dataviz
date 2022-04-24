/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_LIKERT_H__
#define __WISTERIA_LIKERT_H__

#include "barchart.h"

namespace Wisteria::Graphs
    {
    /** @brief A chart which shows the breakdown of Likert-scale survey responses.
        @details Questions' responses can either be plotted as a single bar or split into grouped bar.
         Also, multiple levels of Likert scales are supported.

         | 3-Point Scale   | 7-Point Scale |
         | :-------------- | :-------------------------------- |
         | @image html Likert3Point.svg width=90% | @image html Likert7Point.svg width=90% |

        @par %Data:
         This plot accepts a Data::Dataset where the specified categorical columns represent questions
         and their respective responses. (A string table must be assigned via SetLabels() to these
         columns after import, and that will be used for the legend.) Additionally, a group column
         can optionally be used to subdivide the responses on the chart.

         | GENDER | Question 1 | Question 2 |
         | :--    | --:        | --:        |
         | M      | 1          | 3          |
         | M      | 2          | 2          |
         | F      | 1          | 3          |
         | M      | 3          | 1          |
         | F      | 3          | 2          |
         | M      | 4          | 3          |
         | M      | 3          | 1          |
         | M      | 4          | 4          |
         | M      | 5          | 2          |
         | F      | 3          | 1          |
         | F      | 3          | 3          |
         | F      | 2          | 4          |

         ...
        @par 7-Point Scale Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };
         canvas->SetFixedObjectsGridSize(1, 2);

         // import the dataset (this is available in the "datasets" folder)
         auto surveyData = std::make_shared<Data::Dataset>();
         surveyData->ImportCSV(L"Graph Library Survey.csv",
             Data::ImportInfo().
             CategoricalColumns(
                    {
                    { L"Gender" },
                    { L"I am happy with my current graphics library",
                      CategoricalImportMethod::ReadAsIntegers },
                    { L"Customization is important to me",
                        CategoricalImportMethod::ReadAsIntegers },
                    { L"A simple API is important to me",
                        CategoricalImportMethod::ReadAsIntegers },
                    { L"Support for obscure graphs is important to me",
                        CategoricalImportMethod::ReadAsIntegers },
                    { L"Extensibility is important to me",
                        CategoricalImportMethod::ReadAsIntegers },
                    { L"Standard, \"out-of-the-box\" graph support is important to me",
                        CategoricalImportMethod::ReadAsIntegers },
                    { L"Data importing features are important to me",
                        CategoricalImportMethod::ReadAsIntegers }
                    }) );

         // Although we know that the data is SevenPointCategorized (i.e., 7-point), we can
         // also deduce the scale from the data this way:
         //
         // const auto likertScale = LikertChart::DeduceScale(surveyData
         //                                       surveyData->GetCategoricalColumnNames());
         //
         // Then, "likertScale" can be passed to LikertChart's constructor
         // (and also CreateLabels(), see below).

         // create labels for the responses
         const ColumnWithStringTable::StringTableType codes =
            {
                { 0, L"No response" },
                { 1, L"Strongly disagree" },
                { 2, L"Disagree" },
                { 3, L"Mildly disagree" },
                { 4, L"Neutral" },
                { 5, L"Mildly agree" },
                { 6, L"Agree" },
                { 7, L"Strongly agree" }
            };
         // stock labels can also be created this way:
         //
         // const ColumnWithStringTable::StringTableType codes =
         //     LikertChart::CreateLabels(
         //         LikertChart::LikertSurveyQuestionFormat::SevenPointCategorized);

         auto categoricalNames{ surveyData->GetCategoricalColumnNames() };
         Dataset::RemoveColumnNamesFromList(categoricalNames, { L"Gender"} );
         LikertChart::SetLabels(surveyData, categoricalNames, codes);

         auto likertChart = std::make_shared<LikertChart>(canvas,
             LikertChart::LikertSurveyQuestionFormat::SevenPointCategorized);
         likertChart->SetData(surveyData,
                              categoricalNames,
                              L"Gender");
         // groups with lower responses will have narrower bars
         likertChart->SetBarSizesToRespondentSize(true);

         canvas->SetFixedObject(0, 0, likertChart);
         canvas->SetFixedObject(0, 1, likertChart->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph));
        @endcode

        @par 3-Point Scale Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };
         // we will use a 3-point scale, so a legend isn't really needed
         canvas->SetFixedObjectsGridSize(1, 1);

         // import the dataset (this is available in the "datasets" folder)
         auto surveyData = std::make_shared<Data::Dataset>();
         surveyData->ImportCSV(L"Graph Library Survey.csv",
             Data::ImportInfo().
             CategoricalColumns(
                    {
                    { L"I am happy with my current graphics library",
                      CategoricalImportMethod::ReadAsIntegers },
                    { L"Customization is important to me",
                        CategoricalImportMethod::ReadAsIntegers },
                    { L"A simple API is important to me",
                        CategoricalImportMethod::ReadAsIntegers },
                    { L"Support for obscure graphs is important to me",
                        CategoricalImportMethod::ReadAsIntegers },
                    { L"Extensibility is important to me",
                        CategoricalImportMethod::ReadAsIntegers },
                    { L"Standard, \"out-of-the-box\" graph support is important to me",
                        CategoricalImportMethod::ReadAsIntegers },
                    { L"Data importing features are important to me",
                        CategoricalImportMethod::ReadAsIntegers }
                    }) );
         // Original data has a scale going from 1-7, but we want to simplify
         // it to 1-3. To do this, we will collapse all the positive levels
         // into one, and all negative levels into another level.
         const auto responsesScale = LikertChart::Simplify(surveyData,
                                                 surveyData->GetCategoricalColumnNames(),
                                                 LikertChart::LikertSurveyQuestionFormat::SevenPoint);

         // Simplify() will use stock labels for the responses.
         // To change these, do the following:
         // const ColumnWithStringTable::StringTableType codes =
         //  {
         //     { 0, L"No answer" },
         //     { 1, L"Negative" },
         //     { 2, L"Neither" },
         //     { 3, L"Positive" }
         // };
         // LikertChart::SetLabels(surveyData,
         //                        surveyData->GetCategoricalColumnNames(),
         //                        codes);

         auto likertChart = std::make_shared<LikertChart>(canvas,
            LikertChart::LikertSurveyQuestionFormat::ThreePoint);
         likertChart->SetData(surveyData,
                              surveyData->GetCategoricalColumnNames(),);

         canvas->SetFixedObject(0, 0, likertChart);
        @endcode
        @par Citation:
         Inspired by the following article:

         https://www.airweb.org/article/2021/08/20/data-visualization-quick-tips*/
    class LikertChart final : public BarChart
        {
    private:
        /** @brief A three-point (e.g., disagree, neutral, or agree) Likert question.
            @details This will be the question, how many responses it got, and the number of responses at each point/level.
            @note The neutral and non-response categories are optional. If all responses entered have 0 for either of these categories,
             then their respective sections won't appear on the chart.*/
        class LikertThreePointSurveyQuestion
            {
            friend class LikertChart;
            friend class LikertCategorizedThreePointSurveyQuestion;
        public:
            /// @brief Constructor.
            /// @param question The question (or categorical label).
            /// @param negativeCount The number of negative responses.
            /// @param neutralCount The number of neutral responses.
            /// @param positiveCount The number of positive responses.
            /// @param naCount The number of non-responses.
            LikertThreePointSurveyQuestion(const wxString& question,
                const size_t negativeCount, const size_t neutralCount,
                const size_t positiveCount, const size_t naCount = 0) : m_question(question)
                {
                m_responses = negativeCount+positiveCount+neutralCount+naCount;
                m_negativeRate = round(safe_divide<double>(negativeCount, m_responses)*100);
                m_neutralRate = round(safe_divide<double>(neutralCount, m_responses)*100);
                m_positiveRate = round(safe_divide<double>(positiveCount, m_responses) * 100);
                // those who left the question blank
                m_naRate = round(safe_divide<double>(naCount, m_responses)*100);
                }
            /// @private
            /// @note This is using locale-sensitive sorting.
            [[nodiscard]] bool operator<(const LikertThreePointSurveyQuestion& that) const
                {
                return (wxUILocale::GetCurrent().CompareStrings(
                    m_question, that.m_question, wxCompare_CaseInsensitive) < 0);
                }
        private:
            wxString m_question;
            size_t m_responses{ 0 };
            double m_negativeRate{ 0 };
            double m_positiveRate{ 0 };
            double m_neutralRate{ 0 };
            double m_naRate{ 0 };
            };

        /** @brief A three-point (e.g., disagree, neutral, or agree) Likert question,
             but also includes a categorical breakdown.
            @details This will be the question, how many responses it got,
             and the number of responses at each point/level for each category
             (all under the main question).
            @note The neutral and non-response categories are optional.
             If all responses entered have 0 for either of these categories,
             then their respective sections won't appear on the chart.*/
        class LikertCategorizedThreePointSurveyQuestion
            {
            friend class LikertChart;
        public:
            /// @private
            explicit LikertCategorizedThreePointSurveyQuestion(const wxString& question) : m_question(question)
                {}
            /// @brief Adds a series of responses for a category (e.g., female responses).
            /// @param categoricalResponse The responses and their categorical label.
            /// @note For this response object, the "question" field will be used as the categorical label.
            void AddCategoricalResponse(const LikertThreePointSurveyQuestion& categoricalResponse)
                {
                m_threePointCategories.insert(categoricalResponse);
                m_responses += categoricalResponse.m_responses;
                }
        private:
            wxString m_question;
            size_t m_responses{ 0 };
            std::multiset<LikertThreePointSurveyQuestion> m_threePointCategories;
            };

        /** @brief A five-point (e.g., strongly disagree, disagree, neutral, agree, strongly agree) Likert question.
            @details This will be the question, how many responses it got, and the number of responses at each point/level.
            @note The neutral and non-response categories are optional. If all responses entered have 0 for either of these categories,
             then their respective sections won't appear on the chart.*/
        class LikertFivePointSurveyQuestion
            {
            friend class LikertChart;
            friend class LikertCategorizedFivePointSurveyQuestion;
        public:
            /// @brief Constructor.
            /// @param question The question (or categorical label).
            /// @param negative1Count The number of (strongly) negative responses.
            /// @param negative2Count The number of (somewhat) negative responses.
            /// @param neutralCount The number of neutral responses.
            /// @param positive1Count The number of (somewhat) positive responses.
            /// @param positive2Count The number of (strongly) positive responses.
            /// @param naCount The number of non-responses.
            LikertFivePointSurveyQuestion(const wxString& question,
                const size_t negative1Count, const size_t negative2Count,
                const size_t neutralCount,
                const size_t positive1Count, const size_t positive2Count,
                const size_t naCount = 0) : m_question(question)
                {
                m_responses = negative1Count+negative2Count+positive1Count+positive2Count+neutralCount+naCount;
                m_negative1Rate = round(safe_divide<double>(negative1Count, m_responses)*100);
                m_negative2Rate = round(safe_divide<double>(negative2Count, m_responses)*100);
                m_neutralRate = round(safe_divide<double>(neutralCount, m_responses)*100);
                m_positive1Rate = round(safe_divide<double>(positive1Count, m_responses)*100);
                m_positive2Rate = round(safe_divide<double>(positive2Count, m_responses)*100);
                // those who left the question blank
                m_naRate = round(safe_divide<double>(naCount, m_responses)*100);
                }
            /// @private
            [[nodiscard]] bool operator<(const LikertFivePointSurveyQuestion& that) const
                {
                return (wxUILocale::GetCurrent().CompareStrings(
                    m_question, that.m_question, wxCompare_CaseInsensitive) < 0);
                }
        private:
            wxString m_question;
            size_t m_responses{ 0 };
            double m_negative1Rate{ 0 };
            double m_negative2Rate{ 0 };
            double m_positive1Rate{ 0 };
            double m_positive2Rate{ 0 };
            double m_neutralRate{ 0 };
            double m_naRate{ 0 };
            };

        /** @brief A five-point (e.g., strongly disagree, disagree, neutral, agree, strongly agree) Likert question,
             but also includes a categorical breakdown.
            @details This will be the question, how many responses it got, and the number of responses at each point/level
             for each category (all under the main question).
            @note The neutral and non-response categories are optional. If all responses entered have 0 for either of these categories,
             then their respective sections won't appear on the chart.*/
        class LikertCategorizedFivePointSurveyQuestion
            {
            friend class LikertChart;
        public:
            /// @private
            explicit LikertCategorizedFivePointSurveyQuestion(const wxString& question) : m_question(question)
                {}
            /// @brief Adds a series of responses for a category (e.g., female responses).
            /// @param categoricalResponse The responses and their categorical label.
            /// @note For this response object, the "question" field will be used as the categorical label.
            void AddCategoricalResponse(const LikertFivePointSurveyQuestion& categoricalResponse)
                {
                m_fivePointCategories.insert(categoricalResponse);
                m_responses += categoricalResponse.m_responses;
                }
        private:
            wxString m_question;
            size_t m_responses{ 0 };
            std::multiset<LikertFivePointSurveyQuestion> m_fivePointCategories;
            };

        /** @brief A seven-point (e.g., strongly disagree, disagree, somewhat disagree, neutral,
             strongly agree, agree, somewhat agree) Likert question.
            @details This will be the question, how many responses it got, and the number of responses at each point/level.
            @note The neutral and non-response categories are optional. If all responses entered have 0 for either of these categories,
             then their respective sections won't appear on the chart.*/
        class LikertSevenPointSurveyQuestion
            {
            friend class LikertCategorizedSevenPointSurveyQuestion;
            friend class LikertChart;
        public:
            /// @brief Constructor.
            /// @param question The question (or categorical label).
            /// @param negative1Count The number of (strongly) negative responses.
            /// @param negative2Count The number of negative responses.
            /// @param negative3Count The number of (somewhat) negative responses.
            /// @param neutralCount The number of neutral responses.
            /// @param positive1Count The number of (somewhat) positive responses.
            /// @param positive2Count The number of positive responses.
            /// @param positive3Count The number of (strongly) positive responses.
            /// @param naCount The number of non-responses.
            LikertSevenPointSurveyQuestion(const wxString& question,
                const size_t negative1Count, const size_t negative2Count, const size_t negative3Count,
                const size_t neutralCount,
                const size_t positive1Count, const size_t positive2Count, const size_t positive3Count,
                const size_t naCount = 0) : m_question(question)
                {
                m_responses = negative1Count+negative2Count+negative3Count+positive1Count+positive2Count+positive3Count+neutralCount+naCount;
                m_negative1Rate = round(safe_divide<double>(negative1Count, m_responses)*100);
                m_negative2Rate = round(safe_divide<double>(negative2Count, m_responses)*100);
                m_negative3Rate = round(safe_divide<double>(negative3Count, m_responses)*100);
                m_neutralRate = round(safe_divide<double>(neutralCount, m_responses)*100);
                m_positive1Rate = round(safe_divide<double>(positive1Count, m_responses)*100);
                m_positive2Rate = round(safe_divide<double>(positive2Count, m_responses)*100);
                m_positive3Rate = round(safe_divide<double>(positive3Count, m_responses)*100);
                // those who left the question blank
                m_naRate = round(safe_divide<double>(naCount, m_responses)*100);
                }
            /// @private
            [[nodiscard]] bool operator<(const LikertSevenPointSurveyQuestion& that) const
                {
                return (wxUILocale::GetCurrent().CompareStrings(m_question, that.m_question, wxCompare_CaseInsensitive) < 0);
                }
        private:
            wxString m_question;
            size_t m_responses{ 0 };
            double m_negative1Rate{ 0 };
            double m_negative2Rate{ 0 };
            double m_negative3Rate{ 0 };
            double m_positive1Rate{ 0 };
            double m_positive2Rate{ 0 };
            double m_positive3Rate{ 0 };
            double m_neutralRate{ 0 };
            double m_naRate{ 0 };
            };

        /** @brief A seven-point (e.g., strongly disagree, disagree, somewhat disagree, neutral,
             strongly agree, agree, somewhat agree) Likert question, but also includes a categorical breakdown.
            @details This will be the question, how many responses it got, and the number of responses at each point/level
             for each category (all under the main question).
            @note The neutral and non-response categories are optional. If all responses entered have 0 for either of these categories,
             then their respective sections won't appear on the chart.*/
        class LikertCategorizedSevenPointSurveyQuestion
            {
            friend class LikertChart;
        public:
            /// @private
            explicit LikertCategorizedSevenPointSurveyQuestion(const wxString& question) : m_question(question)
                {}
            /// @brief Adds a series of responses for a category (e.g., female responses).
            /// @param categoricalResponse The responses and their categorical label.
            /// @note For this response object, the "question" field will be used as the categorical label.
            void AddCategoricalResponse(const LikertSevenPointSurveyQuestion& categoricalResponse)
                {
                m_sevenPointCategories.insert(categoricalResponse);
                m_responses += categoricalResponse.m_responses;
                }
        private:
            wxString m_question;
            size_t m_responses{ 0 };
            std::multiset<LikertSevenPointSurveyQuestion> m_sevenPointCategories;
            };
    public:
        /// @brief The type of responses to a Likert survey question.
        enum class LikertSurveyQuestionFormat
            {
            TwoPoint,              /*!< Negative and positive responses.*/
            TwoPointCategorized,   /*!< Negative and positive responses, with sub categories (e.g., male vs. female).*/
            ThreePoint,            /*!< Negative, neutral, and positive responses.*/
            ThreePointCategorized, /*!< Negative, neutral, and positive responses, with sub categories (e.g., male vs. female).*/
            FourPoint,             /*!< Strong negative, negative, neutral, positive, and strong positive responses.*/
            FourPointCategorized,  /*!< Strong negative, negative, positive, and strong positive responses, with sub categories (e.g., male vs. female).*/
            FivePoint,             /*!< Strong negative, negative, neutral, positive, and strong positive responses.*/
            FivePointCategorized,  /*!< Strong negative, negative, neutral, positive, and strong positive responses, with sub categories (e.g., male vs. female).*/
            SixPoint,              /*!< Strong negative, negative, weak negative, strong positive, positive, and weak positive responses.*/
            SixPointCategorized,   /*!< Strong negative, negative, weak negative, strong positive, positive, and weak positive responses, with sub categories (e.g., male vs. female).*/
            SevenPoint,            /*!< Strong negative, negative, weak negative, neutral, strong positive, positive, and weak positive responses.*/
            SevenPointCategorized  /*!< Strong negative, negative, weak negative, neutral, strong positive, positive, and weak positive responses, with sub categories (e.g., male vs. female).*/
            };

        /// @brief Constructor.
        /// @param canvas The canvas that the chart is plotted on.
        /// @param type The survey format.
        /// @param negativeColor The negative color (set to @c wxNullColour to use the default color).
        /// @param positiveColor The positive color (set to @c wxNullColour to use the default color).
        /// @param neutralColor The neutral color (set to @c wxNullColour to use the default color).
        /// @param noResponseColor The non-responses color (set to @c wxNullColour to use the default color).
        /// @note If the Likert scale has more than three levels, the extended levels of positive and negative responses (e.g., "strongly agree")
        ///  will be shades or tints of the base color. For example, if negative is set to red, then other levels of negative will be tinted versions of red.
        LikertChart(Wisteria::Canvas* canvas, const LikertSurveyQuestionFormat type,
                    const wxColour negativeColor = wxNullColour,
                    const wxColour positiveColor = wxNullColour,
                    const wxColour neutralColor = wxNullColour,
                    const wxColour noResponseColor = wxNullColour);

        /** @brief Adds questions (and their responses) to the chart.
            @details The data is analyzed as such:
             - A question (and respective responses) are constructed from each categorical column specified.
               - The question is pulled from the column's title.
               - The responses are pulled from the column's values, which are integral codes. The value 0 represents non-responses,
                 then 1 through the point size are the responses. For example, 1-7 for a 7-point scale, 1-3 for a 3-point scale.
                 1 represents the strongest negative response (e.g., "strongly disagree") and the highest value is the strongest positive response.
               - The string table connected to last categorical column read will be the labels representing the codes (e.g., "Unlikely", "Agree").
                 These are used for the legend. It is required that all categorical columns use the same labels in their string tables.
             - The grouping column is used for grouping the responses. (This only applies if using a categorized chart type.)
             This will split the question row into smaller rows, one for each category.
             An example of this would be demographic labels for the respondents.
             The values in the categorical columns should be coded as 1-7 (or depending on how high the scale is) and should be imported
             using @c CategoricalImportMethod::ReadAsIntegers. Then, you should assign your string table to these columns via SetLabels().

             Note that missing responses in the categorical columns can either be blank or coded as zero.
            @param data %Data containing the responses.
            @param questionColumns The vector of categorical columns to use as questions.
            @param groupColumnName The (optional) group column.
            @sa SetLabels().
            @note Grouping is used if the chart type is categorized (see GetSurveyType()).
            @warning The string tables in the categorical columns need to be synchronized prior to calling this. In other works,
             ensure that the columns use the same string and integral code assignments.
             This should be done after the data is imported and prior to calling this function.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
            std::vector<wxString>& questionColumns,
            std::optional<wxString> groupColumnName = std::nullopt);

        /** @brief Sets a common string table to the specified categorical columns (i.e., questions) in a dataset.
            @details This should be called after calling SetData().
            @param data The dataset to edit.
            @param questionColumns The vector of categorical columns to edit.
            @param codes The string table to use. This should at least start at 0 (no response) and then go
             from 1 to the highest level of the point scale of the chart.
            @sa CreateLabels().*/
        static void SetLabels(std::shared_ptr<Data::Dataset>& data,
                              std::vector<wxString>& questionColumns,
                              const Data::ColumnWithStringTable::StringTableType& codes);

        /** @brief Creates a stock list of labels to use for a particular Likert scale.
            @note DeduceScale() can help with determining the data's Likert scale if unknown, and
             the result for this function can then be used for SetLabels().
            @param type The Likert scale to create labels for.
            @returns A string table with a stock set of labels for the given Likert scale.*/
        [[nodiscard]] static Data::ColumnWithStringTable::StringTableType CreateLabels(const LikertSurveyQuestionFormat& type);

        /** @brief Determines which type of scale (e.g., 1-5) the data is using.
            @details Call this prior to constructing a LikertChart object to help deduce what type of scale to use.
            @param data The dataset to review.
            @param questionColumns The vector of categorical columns to use as questions.
            @param groupColumnName The (optional) grouping column to help deduce if this scale could be categorized.
            @note This will look at the categorical columns in the dataset to deduce the most appropriate scale.
             Also, if the grouping column has more than one unique code in it, then something like
             SevenPoint or SevenPointCategorized will be returned.
            @returns The most likely Likert scale based on the values in the categorical columns.
            @warning This will look at the most extreme values in the responses to deduce the scale.
             This means that if the data is really a seven-point scale but only has responses going up to 5,
             then it will return @c FivePoint. This also assumes that all the response columns are the same scale
             and will return the highest scale from the data that it finds.
             Also, this will throw an exception if any response is higher than 7.*/
        [[nodiscard]] static LikertSurveyQuestionFormat DeduceScale(
            const std::shared_ptr<Data::Dataset>& data,
            std::vector<wxString>& questionColumns,
            std::optional<wxString> groupColumnName = std::nullopt);

        /** @brief Collapses the data into the simplest scale
             (either 3- or 2-point, depending on whether there is a neutral level).
            @details This will also set the string tables for the responses to the simpler scale,
             although SetLabels() can be called afterwards if you wish to customize these labels further.

             - For 4-point scales, this collapses all negative levels to 1 and all positive levels to 2.
             This assumes that all categorical columns (i.e., questions) are coded 0-4
             (0 = no response, 1-2 = negative levels, and 3-4 = positive levels).
             - For 5-point scales, this collapses all negative levels to 1 and all positive levels to 3.
             This assumes that all categorical columns (i.e., questions) are coded 0-5
             (0 = no response, 1-2 = negative levels, 3 = neutral, and 4-5 = positive levels).
             - This 6-point scales, this collapses all negative levels to 1 and all positive levels to 2.
             This assumes that all categorical columns (i.e., questions) are coded 0-6
             (0 = no response, 1-3 = negative levels, and 4-6 = positive levels).
             - For 7-point scales, this collapses all negative levels to 1 and all positive levels to 3.
             This assumes that all categorical columns (i.e., questions) are coded 0-7
             (0 = no response, 1-3 = negative levels, 4 = neutral, and 5-7 = positive levels).

            @param data The dataset to simplify/collapse.
            @param questionColumns The vector of categorical columns to use as questions.
            @param currentFormat The questions' Likert scale.
            @returns The questions' new Likert scale (should be passed to the chart's constructor).
            @note If the data's scale is already 3- or 2-point, then the data will stay the same but the
             question (i.e., categorical) columns' string tables will be reset to use the respective stock labels.*/
        [[nodiscard]] static LikertSurveyQuestionFormat Simplify(std::shared_ptr<Data::Dataset>& data,
            std::vector<wxString>& questionColumns,
            LikertSurveyQuestionFormat currentFormat);

        /// @name Chart Type Functions
        /// @brief Functions relating to the chart's design (e.g., point scale, whether responses are grouped).
        /// @{

        /// @returns The type of questions used for this survey.
        [[nodiscard]] LikertSurveyQuestionFormat GetSurveyType() const noexcept
            { return m_surveyType; }

        /** @brief Gets the number of levels in the survey (e.g., ThreePoint -> 3).
            @returns The number of levels in the survey.*/
        [[nodiscard]] size_t GetLevelCount() const noexcept
            {
            return (GetSurveyType() == LikertSurveyQuestionFormat::TwoPoint ||
                    GetSurveyType() == LikertSurveyQuestionFormat::TwoPointCategorized) ? 2 :
                   (GetSurveyType() == LikertSurveyQuestionFormat::ThreePoint ||
                    GetSurveyType() == LikertSurveyQuestionFormat::ThreePointCategorized) ? 3 :
                   (GetSurveyType() == LikertSurveyQuestionFormat::FourPoint ||
                    GetSurveyType() == LikertSurveyQuestionFormat::FourPointCategorized) ? 4 :
                   (GetSurveyType() == LikertSurveyQuestionFormat::FivePoint ||
                    GetSurveyType() == LikertSurveyQuestionFormat::FivePointCategorized) ? 5 :
                   (GetSurveyType() == LikertSurveyQuestionFormat::SixPoint ||
                    GetSurveyType() == LikertSurveyQuestionFormat::SixPointCategorized) ? 6 :
                   (GetSurveyType() == LikertSurveyQuestionFormat::SevenPoint ||
                    GetSurveyType() == LikertSurveyQuestionFormat::SevenPointCategorized) ? 7 :7;
            }

        /// @brief Gets whether the chart type is categorized
        ///  (i.e., responses are split into groups for each question).
        /// @returns Whether the responses are categorized.
        [[nodiscard]] bool IsCategorized() const noexcept
            {
            return IsCategorized(GetSurveyType());
            }
        /// @}

        /// @name Section Header Functions
        /// @brief Functions relating to the section headers.
        /// @note The neutral and non-response headers are controlled via the string table
        ///  of the last series of responses added (see AddSurveyQuestion()). The label associated
        ///  with 0 will be the non-response label, and the label connected to the value at the middle
        ///  of the range (e.g., 3 if using a 5-point scale) will be the neutral label.
        /// @{

        /// @brief Show section headers (e.g., "Positive" for positive responses).
        /// @param show Whether to show the headers.
        void ShowSectionHeaders(const bool show) noexcept
            { m_showSectionHeaders = show; }
        /// @returns Whether headers are being shown above the bars.
        [[nodiscard]] bool IsShowingSectionHeaders() const noexcept
            { return m_showSectionHeaders; }
        /// @brief Gets the label displayed about the positive response area.
        /// @returns The positive section label.
        [[nodiscard]] const wxString& GetPositiveHeader() const noexcept
            { return m_positiveHeaderLabel; }
        /// @brief Sets the positive area section header.
        /// @param label The label to display.
        void SetPositiveHeader(const wxString& label)
            {
            if (label.length())
                { m_positiveHeaderLabel = label; }
            }
        /// @brief Gets the label displayed about the negative response area.
        /// @returns The negative section label.
        [[nodiscard]] const wxString& GetNegativeHeader() const noexcept
            { return m_negativeHeaderLabel; }
        /// @brief Sets the negative area section header.
        /// @param label The label to display.
        void SetNegativeHeader(const wxString& label)
            {
            if (label.length())
                { m_negativeHeaderLabel = label; }
            }
        /// @returns The no-response label.
        [[nodiscard]] const wxString& GetNoResponseHeader() const noexcept
            { return m_noHeaderLabel; }
        /// @brief Sets the no-response section header.
        /// @param label The label to display.
        void SetNoResponseHeader(const wxString& label)
            {
            if (label.length())
                { m_noHeaderLabel = label; }
            }
        /// @}

        /// @name Bar & Label Functions
        /// @brief Functions relating to how the bars and labels are displayed.
        /// @{

        /// @brief Show response counts next to each question.
        /// @param show Whether to show the counts.
        void ShowResponseCounts(const bool show) noexcept
            { m_showResponseCounts = show; }
        /// @returns Whether response counts are being shown next to each question.
        [[nodiscard]] bool IsShowingResponseCounts() const noexcept
            { return m_showResponseCounts; }

        /// @brief Show percentages on the bars.
        /// @param show Whether to show the percentages.
        void ShowPercentages(const bool show) noexcept
            { m_showPercentages = show; }
        /// @returns Whether percentages are being shown on the bars.
        [[nodiscard]] bool IsShowingPercentages() const noexcept
            { return m_showPercentages; }

        /// @brief Sets bars' widths to be relative to their number of responses.
        /// @details Only applies to categorized charts.
        /// @param adjust Whether to adjust the bars' widths.
        /// @note For categorized plots, the bars' widths are based on the size of the
        ///  category with the most responses. (The category with the most responses will be a full-width
        ///  bar, and the others are scaled to that.)
        void SetBarSizesToRespondentSize(const bool adjust) noexcept
            { m_adjustBarWidthsToRespondentSize = adjust; }
        /// @returns Whether the bars' width are relative to their number of responses.
        [[nodiscard]] bool IsSettingBarSizesToRespondentSize() const noexcept
            { return m_adjustBarWidthsToRespondentSize; }
        /// @}

        /// @brief A bracket going from one question to another.
        struct QuestionsBracket
            {
            /// @brief The first question to start the bracket at.
            wxString m_question1;
            /// @brief The second question to end the bracket at.
            wxString m_question2;
            /// @brief The label for the bracket.
            wxString m_title;
            };

        /// @brief Adds a bracket to a group of questions.
        /// @param qBracket The bracket information, include the start and end questions
        ///  and the bracket title.
        void AddQuestionsBracket(const QuestionsBracket& qBracket)
            { m_questionBrackets.push_back(qBracket); }

        /// @brief Builds and returns a legend using the current colors and labels.
        /// @details This can be then be managed by the parent canvas and placed next to the plot.
        /// @param hint A hint about where the legend will be placed after construction. This is used
        ///  for defining the legend's padding, outlining, canvas proportions, etc.
        /// @returns The legend for the chart.
        /// @note Be sure to set the labels in the dataset prior to calling SetData() if you plan
        ///  create a legend. Refer to SetLabels(), CreateLabels(), and Simplify() for details.
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateLegend(
                                                         const LegendCanvasPlacementHint hint);
    private:
        /** @brief Determines if a format is categorized (i.e., using a grouping variable).
            @param format The format to review.
            @returns `true` if the specified format is categorized.*/
        [[nodiscard]] static bool IsCategorized(const LikertSurveyQuestionFormat format) noexcept;
        /// @brief Draws the brackets connected to questions.
        void AddQuestionBrackets();
        /** @brief Converts a 4-point scale dataset to 2-point.
            @details Basically, this collapses all negative levels to 1 and all positive levels to 2.
             This assumes that all categorical columns (i.e., questions) are coded 0-4
             (0 = no response, 1-2 = negative levels, and 3-4 = positive levels).

            @param data The dataset to collapse.
            @param questionColumns The vector of categorical columns to use as questions.
            @param condensedCodes The simplified string table to use. This should include values 0-2
             and their respective labels (no response, negative, neutral, positive).
            @warning This will overwrite all string tables in the dataset.
            @par Example:
            @code
            LikertChart::Collapse4PointsTo2(data,
                {
                    { 0, L"No response" },
                    { 1, L"Disagree" },
                    { 2, L"Agree" }
                });
            @endcode*/
        static void Collapse4PointsTo2(std::shared_ptr<Data::Dataset>& data,
                                       std::vector<wxString>& questionColumns,
                                       const Data::ColumnWithStringTable::StringTableType& condensedCodes);
        /** @brief Converts a 5-point scale dataset to 3-point.
            @details Basically, this collapses all negative levels to 1 and all positive levels to 3.
             This assumes that all categorical columns (i.e., questions) are coded 0-5
             (0 = no response, 1-2 = negative levels, 3 = neutral, and 4-5 = positive levels).

            @param data The dataset to collapse.
            @param questionColumns The vector of categorical columns to use as questions.
            @param condensedCodes The simplified string table to use. This should include values 0-3
             and their respective labels (no response, negative, neutral, positive).
            @warning This will overwrite all string tables in the dataset.
            @par Example:
            @code
            LikertChart::Collapse7PointsTo3(data,
                {
                    { 0, L"No response" },
                    { 1, L"Disagree" },
                    { 2, L"Neutral" },
                    { 3, L"Agree" }
                });
            @endcode*/
        static void Collapse5PointsTo3(std::shared_ptr<Data::Dataset>& data,
                                       std::vector<wxString>& questionColumns,
                                       const Data::ColumnWithStringTable::StringTableType& condensedCodes);
        /** @brief Converts a 6-point scale dataset to 2-point.
            @details Basically, this collapses all negative levels to 1 and all positive levels to 2.
             This assumes that all categorical columns (i.e., questions) are coded 0-6
             (0 = no response, 1-3 = negative levels, and 4-6 = positive levels).

            @param data The dataset to collapse.
            @param questionColumns The vector of categorical columns to use as questions.
            @param condensedCodes The simplified string table to use. This should include values 0-2
             and their respective labels (no response, negative, neutral, positive).
            @warning This will overwrite all string tables in the dataset.
            @par Example:
            @code
            LikertChart::Collapse6PointsTo2(data,
                {
                    { 0, L"No response" },
                    { 1, L"Disagree" },
                    { 2, L"Agree" }
                });
            @endcode*/
        static void Collapse6PointsTo2(std::shared_ptr<Data::Dataset>& data,
                                       std::vector<wxString>& questionColumns,
                                       const Data::ColumnWithStringTable::StringTableType& condensedCodes);
        /** @brief Converts a 7-point scale dataset to 3-point.
            @details Basically, this collapses all negative levels to 1 and all positive levels to 3.
             This assumes that all categorical columns (i.e., questions) are coded 0-7
             (0 = no response, 1-3 = negative levels, 4 = neutral, and 5-7 = positive levels).

            @param data The dataset to collapse.
            @param questionColumns The vector of categorical columns to use as questions.
            @param condensedCodes The simplified string table to use. This should include values 0-3
             and their respective labels (no response, negative, neutral, positive).
            @warning This will overwrite all string tables in the dataset.
            @par Example:
            @code
            LikertChart::Collapse7PointsTo3(data,
                {
                    { 0, L"No response" },
                    { 1, L"Disagree" },
                    { 2, L"Neutral" },
                    { 3, L"Agree" }
                });
            @endcode*/
        static void Collapse7PointsTo3(std::shared_ptr<Data::Dataset>& data,
                                       std::vector<wxString>& questionColumns,
                                       const Data::ColumnWithStringTable::StringTableType& condensedCodes);
        /** @brief Gets the categorized version of survey format
             (e.g., ThreePoint -> ThreePointCategorized).
            @returns The categorized version of survey format.
            @param format The survey format to make categorized.*/
        [[nodiscard]] static LikertSurveyQuestionFormat MakeFormatCategorized(
            const LikertSurveyQuestionFormat format) noexcept;
        /** @brief Gets the uncategorized version of survey format
             (e.g., ThreePointCategorized -> ThreePoint).
            @returns The uncategorized version of survey format.
            @param format The survey format to make uncategorized.*/
        [[nodiscard]] static LikertSurveyQuestionFormat MakeFormatUncategorized(
            const LikertSurveyQuestionFormat format) noexcept;
        /// @brief Sets the color for the weakest negative point
        ///  (stronger points will be shades of this color).
        /// @param color The color to use.
        void SetNegativeColor(const wxColour color)
            {
            if (color.IsOk())
                { m_negativeColor = color; }
            }
        /// @returns The color used for the weakest (i.e., closest to neutral) negative point.
        [[nodiscard]] wxColour GetNegativeColor() const noexcept
            { return m_negativeColor; }

        /// @brief Sets the color for the neutral bars.
        /// @param color The color to use.
        void SetNeutralColor(const wxColour color)
            {
            if (color.IsOk())
                { m_neutralColor = color; }
            }
        /// @returns The color used for the neutral bars.
        [[nodiscard]] wxColour GetNeutralColor() const noexcept
            { return m_neutralColor; }

        /// @brief Sets the color for the no-response bars.
        /// @param color The color to use.
        void SetNoResponseColor(const wxColour color)
            {
            if (color.IsOk())
                { m_noResponseColor = color; }
            }
        /// @returns The color used for the no-response bars.
        [[nodiscard]] wxColour GetNoResponseColor() const noexcept
            { return m_noResponseColor; }

        /// @brief Sets the color for the weakest positive point
        ///  (higher points will be shades of this color).
        /// @param color The color to use.
        void SetPositiveColor(const wxColour color)
            {
            if (color.IsOk())
                { m_positiveColor = color; }
            }
        /// @returns The color used for the weakest (i.e., closest to neutral) positive point.
        [[nodiscard]] wxColour GetPositiveColor() const noexcept
            { return m_positiveColor; }

        /// @returns The positive response label at a given point.
        /// @param point The positive point label to return.
        ///  Values should be 1-3, going from the weakest positive response to the strongest.
        [[nodiscard]] const wxString& GetPositiveLabel(const size_t point) const noexcept
            {
            wxASSERT_LEVEL_2_MSG(point >= 1 && point <= 3, "Incorrect point specified for label!");
            return (point == 1) ? m_positive1Label :
                (point == 2) ? m_positive2Label :
                (point == 3) ? m_positive3Label :
                m_nullString;
            }
        /// @returns The negative response label at a given point.
        /// @param point The negative point label to return. Values should be 1-3, going from
        ///  the strongest negative response to the weakest.
        [[nodiscard]] const wxString& GetNegativeLabel(const size_t point) const noexcept
            {
            wxASSERT_LEVEL_2_MSG(point >= 1 && point <= 3, "Incorrect point specified for label!");
            return (point == 1) ? m_negative1Label :
                (point == 2) ? m_negative2Label :
                (point == 3) ? m_negative3Label :
                m_nullString;
            }
        /// @returns The neutral response label.
        [[nodiscard]] const wxString& GetNeutralLabel() const noexcept
            { return m_neutralLabel; }

        /// @brief Sets the negative response label at a given point.
        /// @param label The label to display.
        /// @param point The point of negativity (e.g., 1 will be the strongest negative point,
        ///   3 will be the weakest negative).
        /// @note This label will be shown in the legend and also (possibly) as a section header.
        void SetNegativeLabel(const wxString& label, const size_t point)
            {
            wxASSERT_LEVEL_2_MSG(point >= 1 && point <= 3, "Incorrect point specified for label!");
            if (label.empty())
                { return; }
            switch (point)
                {
            case 1:
                m_negative1Label = label;
                break;
            case 2:
                m_negative2Label = label;
                break;
            case 3:
                m_negative3Label = label;
                break;
            default:
                m_negative2Label = label;
                };
            }

        /// @brief Sets the positive response label at a given point.
        /// @param label The label to display.
        /// @param point The point of positivity
        ///  (e.g., 1 will be the weakest positive point [closest one to neutral],
        ///  3 will be the strongest positive).
        /// @note This label will be shown in the legend and also (possibly) as a section header.
        void SetPositiveLabel(const wxString& label, const size_t point)
            {
            wxASSERT_LEVEL_2_MSG(point >= 1 && point <= 3, "Incorrect point specified for label!");
            if (label.empty())
                { return; }
            switch (point)
                {
            case 1:
                m_positive1Label = label;
                break;
            case 2:
                m_positive2Label = label;
                break;
            case 3:
                m_positive3Label = label;
                break;
            default:
                m_positive2Label = label;
                };
            }

        /// @brief Sets the neutral response section header and legend label.
        /// @param label The label to display.
        void SetNeutralLabel(const wxString& label)
            {
            if (label.length())
                { m_neutralLabel = label; }
            }

        /// When categorization is used, we overlay extra bars on top of the categorical responses
        /// to show the questions. Therefore, we keep track of the number of response bars here
        /// so that we don't count the question bars when calculating bar width, line measures, etc.
        /// @returns The number of slots for bars on the plot.
        [[nodiscard]] size_t GetBarSlotCount() const noexcept final
            { return m_responseBarCount; }
        /// Updates the canvas size based on the bar count. This override handles extra bars
        /// needing to be added for categorized plots.
        void UpdateCanvasForBars() final;
        void RecalcSizes(wxDC& dc) final;

        /// @brief Adds a question and its respective responses.
        /// @param question The survey question.
        /// @param responses The responses.
        void AddSurveyQuestion(const wxString& question, const Data::ColumnWithStringTable& responses);
        /// @brief Adds a question and its respective responses.
        /// @param question The survey question.
        /// @param groups The group values column.
        /// @param responses The responses.
        void AddSurveyQuestion(const wxString& question,
                               const Data::ColumnWithStringTable& groups,
                               const Data::ColumnWithStringTable& responses);

        /// @brief Add a three-point (e.g., agree, disagree, or neutral) Likert response.
        /// @param response The question and its breakdowns of the responses.
        /// @note These survey items will be stacked in the order that you call this function
        /// (i.e., the first response will be one at the bottom [at the axes' origin]).
        void AddSurveyQuestion(const LikertThreePointSurveyQuestion& response);
        /// @brief Add a three-point (e.g., agree, disagree, or neutral)
        ///  Likert question with categorical responses.
        /// @param response The question and its categorized breakdowns of the responses.
        /// @note These survey items will be stacked in the order that you call this function
        /// (i.e., the first response will be one at the bottom [at the axes' origin]).
        void AddSurveyQuestion(const LikertCategorizedThreePointSurveyQuestion& response);

        /// @brief Add a five-point (e.g., agree, disagree, or neutral) Likert response.
        /// @param response The question and its breakdowns of the responses.
        /// @note These survey items will be stacked in the order that you call this function
        ///  (i.e., the first response will be one at the bottom [at the axes' origin]).
        void AddSurveyQuestion(const LikertFivePointSurveyQuestion& response);
        /// @brief Add a five-point (e.g., agree, disagree, or neutral)
        ///  Likert question with categorical responses.
        /// @param response The question and its categorized breakdowns of the responses.
        /// @note These survey items will be stacked in the order that you call this function
        ///  (i.e., the first response will be one at the bottom [at the axes' origin]).
        void AddSurveyQuestion(const LikertCategorizedFivePointSurveyQuestion& response);

        /// @brief Add a seven-point (e.g., agree, disagree, or neutral) Likert response.
        /// @param response The question and its breakdowns of the responses.
        /// @note These survey items will be stacked in the order that you call this function
        ///  (i.e., the first response will be one at the bottom [at the axes' origin]).
        void AddSurveyQuestion(const LikertSevenPointSurveyQuestion& response);
        /// @brief Add a seven-point (e.g., agree, disagree, or neutral)
        ///  Likert response with categorical responses.
        /// @param response The question and its breakdowns of the responses.
        /// @note These survey items will be stacked in the order that you call this function
        ///  (i.e., the first response will be one at the bottom [at the axes' origin]).
        void AddSurveyQuestion(const LikertCategorizedSevenPointSurveyQuestion& response);
        void AddSurveyQuestionBar(const LikertThreePointSurveyQuestion& question);
        void AddSurveyQuestionBar(const LikertCategorizedThreePointSurveyQuestion& question);
        void AddSurveyQuestionBar(const LikertFivePointSurveyQuestion& question);
        void AddSurveyQuestionBar(const LikertCategorizedFivePointSurveyQuestion& question);
        void AddSurveyQuestionBar(const LikertSevenPointSurveyQuestion& question);
        void AddSurveyQuestionBar(const LikertCategorizedSevenPointSurveyQuestion& question);

        /** @brief Sets a bar block to be the full width (i.e., 1.0) of a bar slot.
            @details This is used for ensuring that a decal set to SplitTextToFit has all
             the space available.
            @param bar The bar to edit.
            @param barBlockLabel The tag of bar block to edit.*/
        void SetBarBlockFullWidth(Bar& bar, const wxString& tag)
            {
            auto catLabelBlock = bar.FindBlock(tag);
            if (catLabelBlock != bar.GetBlocks().end())
                { catLabelBlock->SetCustomWidth(1); }
            }
        /// @brief Tag for category label bar block.
        [[nodiscard]] wxString GetCategoryBlockLabel() const
            { return _DT(L"CATEGORY_LABEL", DTExplanation::InternalKeyword); }
        /// @brief Tag for neutral label bar block.
        [[nodiscard]] wxString GetNeutralBlockLabel() const
            { return _DT(L"NEUTRAL_BLOCK"); }
        /// @brief Tag for neutral label bar block.
        [[nodiscard]] wxString GetQuestionBlockLabel() const
            { return _DT(L"QUESTION_BLOCK"); }

        std::vector<LikertThreePointSurveyQuestion> m_threePointQuestions;
        std::vector<LikertCategorizedThreePointSurveyQuestion> m_threePointCategorizedQuestions;
        std::vector<LikertFivePointSurveyQuestion> m_fivePointQuestions;
        std::vector<LikertCategorizedFivePointSurveyQuestion> m_fivePointCategorizedQuestions;
        std::vector<LikertSevenPointSurveyQuestion> m_sevenPointQuestions;
        std::vector<LikertCategorizedSevenPointSurveyQuestion> m_sevenPointCategorizedQuestions;
        // always present and always a full-scale of 100
        constexpr static double m_questionBlockSize{ 100 };
        // always consumes a fifth of the question block (if being shown)
        constexpr static double m_responseCountBlockSize{ m_questionBlockSize/5 };
        // other block sizes
        double m_categoryBlockSize{ 0 };
        double m_negativeBlockSize{ 0 };
        double m_positiveBlockSize{ 0 };
        double m_neutralBlockSize{ 0 };
        double m_naBlockSize{ 0 };
        double m_neutralMaxSize{ 0 };
        double m_naMaxSize{ 0 };

        size_t m_responseBarCount{ 0 };
        size_t m_maxResondants{ 0 };

        bool m_showResponseCounts{ false };
        bool m_showPercentages{ true };
        bool m_showSectionHeaders{ true };
        bool m_adjustBarWidthsToRespondentSize{ false };

        wxColour m_negativeColor{ Colors::ColorBrewer::GetColor(Colors::Color::Orange) };
        wxColour m_positiveColor{ Colors::ColorBrewer::GetColor(Colors::Color::Cerulean) };
        wxColour m_neutralColor{ Colors::ColorBrewer::GetColor(Colors::Color::LavenderMist) };
        wxColour m_noResponseColor{ Colors::ColorBrewer::GetColor(Colors::Color::White) };

        // labels displayed on the legend
        wxString m_neutralLabel;
        wxString m_positive1Label;
        wxString m_positive2Label;
        wxString m_positive3Label;
        wxString m_negative1Label;
        wxString m_negative2Label;
        wxString m_negative3Label;
        // header labels (can be customized by user)
        wxString m_positiveHeaderLabel{ _(L"Agree") };
        wxString m_negativeHeaderLabel{ _(L"Disagree") };
        wxString m_noHeaderLabel{ _(L"No Response") };

        wxString m_nullString{ L"" };

        LikertSurveyQuestionFormat m_surveyType{ LikertSurveyQuestionFormat::ThreePoint };

        std::vector<QuestionsBracket> m_questionBrackets;
        };
    }

/** @}*/

#endif //__WISTERIA_LIKERT_H__
