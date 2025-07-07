/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_ENUM_CONVERT_H
#define WISTERIA_ENUM_CONVERT_H

#include "../base/icons.h"
#include "../base/shapes.h"
#include "../base/tablelink.h"
#include "../data/join.h"
#include "../data/pivot.h"
#include "../data/subset.h"
#include "../graphs/boxplot.h"
#include "../graphs/candlestickplot.h"
#include "../graphs/categoricalbarchart.h"
#include "../graphs/ganttchart.h"
#include "../graphs/heatmap.h"
#include "../graphs/histogram.h"
#include "../graphs/likertchart.h"
#include "../graphs/lineplot.h"
#include "../graphs/lrroadmap.h"
#include "../graphs/piechart.h"
#include "../graphs/proconroadmap.h"
#include "../graphs/sankeydiagram.h"
#include "../graphs/table.h"
#include "../graphs/wcurveplot.h"
#include "../graphs/wordcloud.h"
#include "enums.h"
#include <wx/wx.h>

namespace Wisteria
    {
    /// @private
    class ReportEnumConvert
        {
      public:
        //---------------------------------------------------
        [[nodiscard]]
        static std::shared_ptr<Colors::Schemes::ColorScheme>
        ConvertColorScheme(wxString schemeColor)
            {
            static std::map<std::wstring_view, std::shared_ptr<Colors::Schemes::ColorScheme>>
                colorSchemes = {
                    { L"dusk", std::make_shared<Colors::Schemes::Dusk>() },
                    { L"earthtones", std::make_shared<Colors::Schemes::EarthTones>() },
                    { L"decade1920s", std::make_shared<Colors::Schemes::Decade1920s>() },
                    { L"decade1940s", std::make_shared<Colors::Schemes::Decade1940s>() },
                    { L"decade1950s", std::make_shared<Colors::Schemes::Decade1950s>() },
                    { L"decade1960s", std::make_shared<Colors::Schemes::Decade1960s>() },
                    { L"decade1970s", std::make_shared<Colors::Schemes::Decade1970s>() },
                    { L"decade1980s", std::make_shared<Colors::Schemes::Decade1980s>() },
                    { L"decade1990s", std::make_shared<Colors::Schemes::Decade1990s>() },
                    { L"decade2000s", std::make_shared<Colors::Schemes::Decade2000s>() },
                    { L"october", std::make_shared<Colors::Schemes::October>() },
                    { L"slytherin", std::make_shared<Colors::Schemes::Slytherin>() },
                    { L"campfire", std::make_shared<Colors::Schemes::Campfire>() },
                    { L"coffeeshop", std::make_shared<Colors::Schemes::CoffeeShop>() },
                    { L"arcticchill", std::make_shared<Colors::Schemes::ArcticChill>() },
                    { L"backtoschool", std::make_shared<Colors::Schemes::BackToSchool>() },
                    { L"boxofchocolates", std::make_shared<Colors::Schemes::BoxOfChocolates>() },
                    { L"cosmopolitan", std::make_shared<Colors::Schemes::Cosmopolitan>() },
                    { L"dayandnight", std::make_shared<Colors::Schemes::DayAndNight>() },
                    { L"freshflowers", std::make_shared<Colors::Schemes::FreshFlowers>() },
                    { L"icecream", std::make_shared<Colors::Schemes::IceCream>() },
                    { L"urbanoasis", std::make_shared<Colors::Schemes::UrbanOasis>() },
                    { L"typewriter", std::make_shared<Colors::Schemes::Typewriter>() },
                    { L"tastywaves", std::make_shared<Colors::Schemes::TastyWaves>() },
                    { L"spring", std::make_shared<Colors::Schemes::Spring>() },
                    { L"shabbychic", std::make_shared<Colors::Schemes::ShabbyChic>() },
                    { L"rollingthunder", std::make_shared<Colors::Schemes::RollingThunder>() },
                    { L"producesection", std::make_shared<Colors::Schemes::ProduceSection>() },
                    { L"nautical", std::make_shared<Colors::Schemes::Nautical>() },
                    { L"semesters", std::make_shared<Colors::Schemes::Semesters>() },
                    { L"seasons", std::make_shared<Colors::Schemes::Seasons>() },
                    { L"meadowsunset", std::make_shared<Colors::Schemes::MeadowSunset>() }
                };

            const auto foundPos =
                colorSchemes.find(std::wstring_view(schemeColor.MakeLower().wc_str()));
            return (foundPos != colorSchemes.cend() ? foundPos->second : nullptr);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Icons::IconShape> ConvertIcon(wxString iconStr)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring_view, Icons::IconShape> iconEnums = {
                { L"blank", Icons::IconShape::Blank },
                { L"horizontal-line", Icons::IconShape::HorizontalLine },
                { L"arrow-right", Icons::IconShape::ArrowRight },
                { L"circle", Icons::IconShape::Circle },
                { L"image", Icons::IconShape::Image },
                { L"horizontal-separator", Icons::IconShape::HorizontalSeparator },
                { L"horizontal-arrow-right-separator",
                  Icons::IconShape::HorizontalArrowRightSeparator },
                { L"color-gradient", Icons::IconShape::ColorGradient },
                { L"square", Icons::IconShape::Square },
                { L"triangle-upward", Icons::IconShape::TriangleUpward },
                { L"triangle-downward", Icons::IconShape::TriangleDownward },
                { L"triangle-right", Icons::IconShape::TriangleRight },
                { L"triangle-left", Icons::IconShape::TriangleLeft },
                { L"diamond", Icons::IconShape::Diamond },
                { L"plus", Icons::IconShape::Plus },
                { L"asterisk", Icons::IconShape::Asterisk },
                { L"hexagon", Icons::IconShape::Hexagon },
                { L"box-plot", Icons::IconShape::BoxPlot },
                { L"location-marker", Icons::IconShape::LocationMarker },
                { L"go-road-sign", Icons::IconShape::GoRoadSign },
                { L"warning-road-sign", Icons::IconShape::WarningRoadSign },
                { L"sun", Icons::IconShape::Sun },
                { L"flower", Icons::IconShape::Flower },
                { L"fall-leaf", Icons::IconShape::FallLeaf },
                { L"top-curly-brace", Icons::IconShape::TopCurlyBrace },
                { L"right-curly-brace", Icons::IconShape::RightCurlyBrace },
                { L"bottom-curly-brace", Icons::IconShape::BottomCurlyBrace },
                { L"left-curly-brace", Icons::IconShape::LeftCurlyBrace },
                { L"man", Icons::IconShape::Man },
                { L"woman", Icons::IconShape::Woman },
                { L"business-woman", Icons::IconShape::BusinessWoman },
                { L"chevron-downward", Icons::IconShape::ChevronDownward },
                { L"chevron-upward", Icons::IconShape::ChevronUpward },
                { L"text", Icons::IconShape::Text },
                { L"tack", Icons::IconShape::Tack },
                { L"banner", Icons::IconShape::Banner },
                { L"watercolor-rectangle", Icons::IconShape::WaterColorRectangle },
                { L"thick-watercolor-rectangle", Icons::IconShape::ThickWaterColorRectangle },
                { L"graduation-cap", Icons::IconShape::GraduationCap },
                { L"book", Icons::IconShape::Book },
                { L"tire", Icons::IconShape::Tire },
                { L"snowflake", Icons::IconShape::Snowflake },
                { L"newspaper", Icons::IconShape::Newspaper },
                { L"car", Icons::IconShape::Car },
                { L"blackboard", Icons::IconShape::Blackboard },
                { L"clock", Icons::IconShape::Clock },
                { L"ruler", Icons::IconShape::Ruler },
                { L"ivbag", Icons::IconShape::IVBag },
                { L"cold-thermometer", Icons::IconShape::ColdThermometer },
                { L"hot-thermometer", Icons::IconShape::HotThermometer },
                { L"apple", Icons::IconShape::Apple },
                { L"granny-smith-apple", Icons::IconShape::GrannySmithApple },
                { L"heart", Icons::IconShape::Heart },
                { L"immaculate-heart", Icons::IconShape::ImmaculateHeart },
                { L"flame", Icons::IconShape::Flame },
                { L"office", Icons::IconShape::Office },
                { L"factory", Icons::IconShape::Factory },
                { L"house", Icons::IconShape::House },
                { L"barn", Icons::IconShape::Barn },
                { L"farm", Icons::IconShape::Farm },
                { L"dollar", Icons::IconShape::Dollar },
                { L"monitor", Icons::IconShape::Monitor }
            };

            const auto foundPos = iconEnums.find(std::wstring_view(iconStr.MakeLower().wc_str()));
            return (foundPos != iconEnums.cend() ?
                        std::optional<Icons::IconShape>(foundPos->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxPaperSize> ConvertPaperSize(const wxString& value)
            {
            static const std::map<std::wstring, wxPaperSize> paperSizeValues = {
                { L"paper-letter", wxPaperSize::wxPAPER_LETTER },
                { L"paper-legal", wxPaperSize::wxPAPER_LEGAL },
                { L"paper-a4", wxPaperSize::wxPAPER_A4 },
                { L"paper-csheet", wxPaperSize::wxPAPER_CSHEET },
                { L"paper-dsheet", wxPaperSize::wxPAPER_DSHEET },
                { L"paper-esheet", wxPaperSize::wxPAPER_ESHEET },
                { L"paper-lettersmall", wxPaperSize::wxPAPER_LETTERSMALL },
                { L"paper-tabloid", wxPaperSize::wxPAPER_TABLOID },
                { L"paper-ledger", wxPaperSize::wxPAPER_LEDGER },
                { L"paper-statement", wxPaperSize::wxPAPER_STATEMENT },
                { L"paper-executive", wxPaperSize::wxPAPER_EXECUTIVE },
                { L"paper-a3", wxPaperSize::wxPAPER_A3 },
                { L"paper-a4small", wxPaperSize::wxPAPER_A4SMALL },
                { L"paper-a5", wxPaperSize::wxPAPER_A5 },
                { L"paper-b4", wxPaperSize::wxPAPER_B4 },
                { L"paper-b5", wxPaperSize::wxPAPER_B5 },
                { L"paper-folio", wxPaperSize::wxPAPER_FOLIO },
                { L"paper-quarto", wxPaperSize::wxPAPER_QUARTO },
                { L"paper-10x14", wxPaperSize::wxPAPER_10X14 },
                { L"paper-11x17", wxPaperSize::wxPAPER_11X17 },
                { L"paper-note", wxPaperSize::wxPAPER_NOTE },
                { L"paper-env-9", wxPaperSize::wxPAPER_ENV_9 },
                { L"paper-env-10", wxPaperSize::wxPAPER_ENV_10 },
                { L"paper-env-11", wxPaperSize::wxPAPER_ENV_11 },
                { L"paper-env-12", wxPaperSize::wxPAPER_ENV_12 },
                { L"paper-env-14", wxPaperSize::wxPAPER_ENV_14 },
                { L"paper-env-dl", wxPaperSize::wxPAPER_ENV_DL },
                { L"paper-env-c5", wxPaperSize::wxPAPER_ENV_C5 },
                { L"paper-env-c3", wxPaperSize::wxPAPER_ENV_C3 },
                { L"paper-env-c4", wxPaperSize::wxPAPER_ENV_C4 },
                { L"paper-env-c6", wxPaperSize::wxPAPER_ENV_C6 },
                { L"paper-env-c65", wxPaperSize::wxPAPER_ENV_C65 },
                { L"paper-env-b4", wxPaperSize::wxPAPER_ENV_B4 },
                { L"paper-env-b5", wxPaperSize::wxPAPER_ENV_B5 },
                { L"paper-env-b6", wxPaperSize::wxPAPER_ENV_B6 },
                { L"paper-env-italy", wxPaperSize::wxPAPER_ENV_ITALY },
                { L"paper-env-monarch", wxPaperSize::wxPAPER_ENV_MONARCH },
                { L"paper-env-personal", wxPaperSize::wxPAPER_ENV_PERSONAL },
                { L"paper-fanfold-us", wxPaperSize::wxPAPER_FANFOLD_US },
                { L"paper-fanfold-std-german", wxPaperSize::wxPAPER_FANFOLD_STD_GERMAN },
                { L"paper-fanfold-lgl-german", wxPaperSize::wxPAPER_FANFOLD_LGL_GERMAN },
                { L"paper-iso-b4", wxPaperSize::wxPAPER_ISO_B4 },
                { L"paper-japanese-postcard", wxPaperSize::wxPAPER_JAPANESE_POSTCARD },
                { L"paper-9x11", wxPaperSize::wxPAPER_9X11 },
                { L"paper-10x11", wxPaperSize::wxPAPER_10X11 },
                { L"paper-15x11", wxPaperSize::wxPAPER_15X11 },
                { L"paper-env-invite", wxPaperSize::wxPAPER_ENV_INVITE },
                { L"paper-letter-extra", wxPaperSize::wxPAPER_LETTER_EXTRA },
                { L"paper-legal-extra", wxPaperSize::wxPAPER_LEGAL_EXTRA },
                { L"paper-tabloid-extra", wxPaperSize::wxPAPER_TABLOID_EXTRA },
                { L"paper-a4-extra", wxPaperSize::wxPAPER_A4_EXTRA },
                { L"paper-letter-transverse", wxPaperSize::wxPAPER_LETTER_TRANSVERSE },
                { L"paper-a4-transverse", wxPaperSize::wxPAPER_A4_TRANSVERSE },
                { L"paper-letter-extra-transverse", wxPaperSize::wxPAPER_LETTER_EXTRA_TRANSVERSE },
                { L"paper-a-plus", wxPaperSize::wxPAPER_A_PLUS },
                { L"paper-b-plus", wxPaperSize::wxPAPER_B_PLUS },
                { L"paper-letter-plus", wxPaperSize::wxPAPER_LETTER_PLUS },
                { L"paper-a4-plus", wxPaperSize::wxPAPER_A4_PLUS },
                { L"paper-a5-transverse", wxPaperSize::wxPAPER_A5_TRANSVERSE },
                { L"paper-b5-transverse", wxPaperSize::wxPAPER_B5_TRANSVERSE },
                { L"paper-a3-extra", wxPaperSize::wxPAPER_A3_EXTRA },
                { L"paper-a5-extra", wxPaperSize::wxPAPER_A5_EXTRA },
                { L"paper-b5-extra", wxPaperSize::wxPAPER_B5_EXTRA },
                { L"paper-a2", wxPaperSize::wxPAPER_A2 },
                { L"paper-a3-transverse", wxPaperSize::wxPAPER_A3_TRANSVERSE },
                { L"paper-a3-extra-transverse", wxPaperSize::wxPAPER_A3_EXTRA_TRANSVERSE },
                { L"paper-dbl-japanese-postcard", wxPaperSize::wxPAPER_DBL_JAPANESE_POSTCARD },
                { L"paper-a6", wxPaperSize::wxPAPER_A6 },
                { L"paper-jenv-kaku2", wxPaperSize::wxPAPER_JENV_KAKU2 },
                { L"paper-jenv-kaku3", wxPaperSize::wxPAPER_JENV_KAKU3 },
                { L"paper-jenv-chou3", wxPaperSize::wxPAPER_JENV_CHOU3 },
                { L"paper-jenv-chou4", wxPaperSize::wxPAPER_JENV_CHOU4 },
                { L"paper-letter-rotated", wxPaperSize::wxPAPER_LETTER_ROTATED },
                { L"paper-a3-rotated", wxPaperSize::wxPAPER_A3_ROTATED },
                { L"paper-a4-rotated", wxPaperSize::wxPAPER_A4_ROTATED },
                { L"paper-a5-rotated", wxPaperSize::wxPAPER_A5_ROTATED },
                { L"paper-b4-jis-rotated", wxPaperSize::wxPAPER_B4_JIS_ROTATED },
                { L"paper-b5-jis-rotated", wxPaperSize::wxPAPER_B5_JIS_ROTATED },
                { L"paper-japanese-postcard-rotated",
                  wxPaperSize::wxPAPER_JAPANESE_POSTCARD_ROTATED },
                { L"paper-dbl-japanese-postcard-rotated",
                  wxPaperSize::wxPAPER_DBL_JAPANESE_POSTCARD_ROTATED },
                { L"paper-a6-rotated", wxPaperSize::wxPAPER_A6_ROTATED },
                { L"paper-jenv-kaku2-rotated", wxPaperSize::wxPAPER_JENV_KAKU2_ROTATED },
                { L"paper-jenv-kaku3-rotated", wxPaperSize::wxPAPER_JENV_KAKU3_ROTATED },
                { L"paper-jenv-chou3-rotated", wxPaperSize::wxPAPER_JENV_CHOU3_ROTATED },
                { L"paper-jenv-chou4-rotated", wxPaperSize::wxPAPER_JENV_CHOU4_ROTATED },
                { L"paper-b6-jis", wxPaperSize::wxPAPER_B6_JIS },
                { L"paper-b6-jis-rotated", wxPaperSize::wxPAPER_B6_JIS_ROTATED },
                { L"paper-12x11", wxPaperSize::wxPAPER_12X11 },
                { L"paper-jenv-you4", wxPaperSize::wxPAPER_JENV_YOU4 },
                { L"paper-jenv-you4-rotated", wxPaperSize::wxPAPER_JENV_YOU4_ROTATED },
                { L"paper-p16k", wxPaperSize::wxPAPER_P16K },
                { L"paper-p32k", wxPaperSize::wxPAPER_P32K },
                { L"paper-p32kbig", wxPaperSize::wxPAPER_P32KBIG },
                { L"paper-penv-1", wxPaperSize::wxPAPER_PENV_1 },
                { L"paper-penv-2", wxPaperSize::wxPAPER_PENV_2 },
                { L"paper-penv-3", wxPaperSize::wxPAPER_PENV_3 },
                { L"paper-penv-4", wxPaperSize::wxPAPER_PENV_4 },
                { L"paper-penv-5", wxPaperSize::wxPAPER_PENV_5 },
                { L"paper-penv-6", wxPaperSize::wxPAPER_PENV_6 },
                { L"paper-penv-7", wxPaperSize::wxPAPER_PENV_7 },
                { L"paper-penv-8", wxPaperSize::wxPAPER_PENV_8 },
                { L"paper-penv-9", wxPaperSize::wxPAPER_PENV_9 },
                { L"paper-penv-10", wxPaperSize::wxPAPER_PENV_10 },
                { L"paper-p16k-rotated", wxPaperSize::wxPAPER_P16K_ROTATED },
                { L"paper-p32k-rotated", wxPaperSize::wxPAPER_P32K_ROTATED },
                { L"paper-p32kbig-rotated", wxPaperSize::wxPAPER_P32KBIG_ROTATED },
                { L"paper-penv-1-rotated", wxPaperSize::wxPAPER_PENV_1_ROTATED },
                { L"paper-penv-2-rotated", wxPaperSize::wxPAPER_PENV_2_ROTATED },
                { L"paper-penv-3-rotated", wxPaperSize::wxPAPER_PENV_3_ROTATED },
                { L"paper-penv-4-rotated", wxPaperSize::wxPAPER_PENV_4_ROTATED },
                { L"paper-penv-5-rotated", wxPaperSize::wxPAPER_PENV_5_ROTATED },
                { L"paper-penv-6-rotated", wxPaperSize::wxPAPER_PENV_6_ROTATED },
                { L"paper-penv-7-rotated", wxPaperSize::wxPAPER_PENV_7_ROTATED },
                { L"paper-penv-8-rotated", wxPaperSize::wxPAPER_PENV_8_ROTATED },
                { L"paper-penv-9-rotated", wxPaperSize::wxPAPER_PENV_9_ROTATED },
                { L"paper-penv-10-rotated", wxPaperSize::wxPAPER_PENV_10_ROTATED },
                { L"paper-a0", wxPaperSize::wxPAPER_A0 },
                { L"paper-a1", wxPaperSize::wxPAPER_A1 }
            };

            const auto foundValue = paperSizeValues.find(value.Lower().ToStdWstring());
            return ((foundValue != paperSizeValues.cend()) ?
                        std::optional<wxPaperSize>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<LabelPlacement> ConvertLabelPlacement(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, LabelPlacement> labelPlacementValues = {
                { L"next-to-parent", LabelPlacement::NextToParent },
                { L"flush", LabelPlacement::Flush }
            };

            const auto foundValue = labelPlacementValues.find(value.Lower().ToStdWstring());
            return ((foundValue != labelPlacementValues.cend()) ?
                        std::optional<LabelPlacement>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<TextAlignment> ConvertTextAlignment(const wxString& value)
            {
            static const std::map<std::wstring, TextAlignment> textAlignValues = {
                { L"flush-left", TextAlignment::FlushLeft },
                { L"flush-right", TextAlignment::FlushRight },
                { L"ragged-right", TextAlignment::RaggedRight },
                { L"ragged-left", TextAlignment::RaggedLeft },
                { L"centered", TextAlignment::Centered },
                { L"justified", TextAlignment::Justified },
                { L"justified-at-character", TextAlignment::JustifiedAtCharacter },
                { L"justified-at-word", TextAlignment::JustifiedAtWord }
            };

            const auto foundValue = textAlignValues.find(value.Lower().ToStdWstring());
            return ((foundValue != textAlignValues.cend()) ?
                        std::optional<TextAlignment>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<GraphColumnHeader> ConvertGraphColumnHeader(const wxString& value)
            {
            static const std::map<std::wstring, GraphColumnHeader> graphColumnHeader = {
                { L"as-header", GraphColumnHeader::AsHeader },
                { L"as-footer", GraphColumnHeader::AsFooter },
                { L"no-display", GraphColumnHeader::NoDisplay }
            };

            const auto foundValue = graphColumnHeader.find(value.Lower().ToStdWstring());
            return ((foundValue != graphColumnHeader.cend()) ?
                        std::optional<GraphColumnHeader>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<FlowShape> ConvertFlowShape(const wxString& value)
            {
            static const std::map<std::wstring, FlowShape> flowShapeValues = {
                { L"curvy", FlowShape::Curvy }, { L"jagged", FlowShape::Jagged }
            };

            const auto foundValue = flowShapeValues.find(value.Lower().ToStdWstring());
            return ((foundValue != flowShapeValues.cend()) ?
                        std::optional<FlowShape>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<wxBrushStyle> ConvertBrushStyle(const wxString& value)
            {
            static const std::map<std::wstring, wxBrushStyle> styleValues = {
                { L"backwards-diagonal-hatch", wxBrushStyle::wxBRUSHSTYLE_BDIAGONAL_HATCH },
                { L"forward-diagonal-hatch", wxBrushStyle::wxBRUSHSTYLE_FDIAGONAL_HATCH },
                { L"cross-diagonal-hatch", wxBrushStyle::wxBRUSHSTYLE_CROSSDIAG_HATCH },
                { L"solid", wxBrushStyle::wxBRUSHSTYLE_SOLID },
                { L"cross-hatch", wxBrushStyle::wxBRUSHSTYLE_CROSS_HATCH },
                { L"horizontal-hatch", wxBrushStyle::wxBRUSHSTYLE_HORIZONTAL_HATCH },
                { L"vertical-hatch", wxBrushStyle::wxBRUSHSTYLE_VERTICAL_HATCH }
            };

            const auto foundValue = styleValues.find(value.Lower().ToStdWstring());
            return ((foundValue != styleValues.cend()) ?
                        std::optional<wxBrushStyle>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<DateInterval> ConvertDateInterval(const wxString& value)
            {
            static const std::map<std::wstring, DateInterval> dateValues = {
                { L"daily", DateInterval::Daily },
                { L"fiscal-quarterly", DateInterval::FiscalQuarterly },
                { L"monthly", DateInterval::Monthly },
                { L"weekly", DateInterval::Weekly }
            };

            const auto foundValue = dateValues.find(value.Lower().ToStdWstring());
            return ((foundValue != dateValues.cend()) ?
                        std::optional<DateInterval>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<TableCellFormat> ConvertTableCellFormat(const wxString& value)
            {
            static const std::map<std::wstring, TableCellFormat> formatValues = {
                { L"accounting", TableCellFormat::Accounting },
                { L"general", TableCellFormat::General },
                { L"percent", TableCellFormat::Percent },
                { L"percent-changed", TableCellFormat::PercentChange }
            };

            const auto foundValue = formatValues.find(value.Lower().ToStdWstring());
            return ((foundValue != formatValues.cend()) ?
                        std::optional<TableCellFormat>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<FiscalYear> ConvertFiscalYear(const wxString& value)
            {
            static const std::map<std::wstring, FiscalYear> fyValues = {
                { L"education", FiscalYear::Education }, { L"us-business", FiscalYear::USBusiness }
            };

            const auto foundValue = fyValues.find(value.Lower().ToStdWstring());
            return ((foundValue != fyValues.cend()) ?
                        std::optional<FiscalYear>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::GanttChart::TaskLabelDisplay>
        ConvertTaskLabelDisplay(const wxString& value)
            {
            static const std::map<std::wstring, Graphs::GanttChart::TaskLabelDisplay>
                taskLabelDisplays = {
                    { L"days", Graphs::GanttChart::TaskLabelDisplay::Days },
                    { L"description", Graphs::GanttChart::TaskLabelDisplay::Description },
                    { L"description-and-days",
                      Graphs::GanttChart::TaskLabelDisplay::DescriptionAndDays },
                    { L"no-display", Graphs::GanttChart::TaskLabelDisplay::NoDisplay },
                    { L"resource", Graphs::GanttChart::TaskLabelDisplay::Resource },
                    { L"resource-and-days", Graphs::GanttChart::TaskLabelDisplay::ResourceAndDays },
                    { L"resource-and-description",
                      Graphs::GanttChart::TaskLabelDisplay::ResourceAndDescription },
                    { L"resource-description-and-days",
                      Graphs::GanttChart::TaskLabelDisplay::ResourceDescriptionAndDays }
                };

            const auto foundValue = taskLabelDisplays.find(value.Lower().ToStdWstring());
            return ((foundValue != taskLabelDisplays.cend()) ?
                        std::optional<Graphs::GanttChart::TaskLabelDisplay>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::CandlestickPlot::PlotType>
        ConvertCandlestickPlotType(const wxString& value)
            {
            static const std::map<std::wstring, Graphs::CandlestickPlot::PlotType> candleTypes = {
                { L"candlestick", Graphs::CandlestickPlot::PlotType::Candlestick },
                { L"ohlc", Graphs::CandlestickPlot::PlotType::Ohlc }
            };

            const auto foundValue = candleTypes.find(value.Lower().ToStdWstring());
            return ((foundValue != candleTypes.cend()) ?
                        std::optional<Graphs::CandlestickPlot::PlotType>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::LikertChart::LikertSurveyQuestionFormat>
        ConvertLikertSurveyQuestionFormat(const wxString& value)
            {
            static const std::map<std::wstring, Graphs::LikertChart::LikertSurveyQuestionFormat>
                surveyTypes = {
                    { L"two-point", Graphs::LikertChart::LikertSurveyQuestionFormat::TwoPoint },
                    { L"two-point-categorized",
                      Graphs::LikertChart::LikertSurveyQuestionFormat::TwoPointCategorized },
                    { L"three-point", Graphs::LikertChart::LikertSurveyQuestionFormat::ThreePoint },
                    { L"threepoint-categorized",
                      Graphs::LikertChart::LikertSurveyQuestionFormat::ThreePointCategorized },
                    { L"four-point", Graphs::LikertChart::LikertSurveyQuestionFormat::FourPoint },
                    { L"four-point-categorized",
                      Graphs::LikertChart::LikertSurveyQuestionFormat::FourPointCategorized },
                    { L"five-point", Graphs::LikertChart::LikertSurveyQuestionFormat::FivePoint },
                    { L"five-point-categorized",
                      Graphs::LikertChart::LikertSurveyQuestionFormat::FivePointCategorized },
                    { L"six-point", Graphs::LikertChart::LikertSurveyQuestionFormat::SixPoint },
                    { L"six-point-categorized",
                      Graphs::LikertChart::LikertSurveyQuestionFormat::SixPointCategorized },
                    { L"seven-point", Graphs::LikertChart::LikertSurveyQuestionFormat::SevenPoint },
                    { L"seven-point-categorized",
                      Graphs::LikertChart::LikertSurveyQuestionFormat::SevenPointCategorized }
                };

            const auto foundValue = surveyTypes.find(value.Lower().ToStdWstring());
            return ((foundValue != surveyTypes.cend()) ?
                        std::optional<Graphs::LikertChart::LikertSurveyQuestionFormat>(
                            foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<BoxEffect> ConvertBoxEffect(const wxString& value)
            {
            static const std::map<std::wstring_view, BoxEffect> boxEffects = {
                { L"common-image", BoxEffect::CommonImage },
                { L"image", BoxEffect::Image },
                { L"fade-from-bottom-to-top", BoxEffect::FadeFromBottomToTop },
                { L"fade-from-left-to-right", BoxEffect::FadeFromLeftToRight },
                { L"fade-from-right-to-left", BoxEffect::FadeFromRightToLeft },
                { L"fade-from-top-to-bottom", BoxEffect::FadeFromTopToBottom },
                { L"glassy", BoxEffect::Glassy },
                { L"solid", BoxEffect::Solid },
                { L"stipple-image", BoxEffect::StippleImage },
                { L"stipple-shape", BoxEffect::StippleShape },
                { L"watercolor", BoxEffect::WaterColor }
            };

            const auto foundValue = boxEffects.find(value.Lower().ToStdWstring());
            return ((foundValue != boxEffects.cend()) ?
                        std::optional<BoxEffect>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<PieSliceEffect> ConvertPieSliceEffect(const wxString& value)
            {
            static const std::map<std::wstring_view, PieSliceEffect> sliceEffects = {
                { L"image", PieSliceEffect::Image }, { L"solid", PieSliceEffect::Solid }
            };

            const auto foundValue = sliceEffects.find(value.Lower().ToStdWstring());
            return ((foundValue != sliceEffects.cend()) ?
                        std::optional<PieSliceEffect>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Perimeter> ConvertPerimeter(const wxString& value)
            {
            static const std::map<std::wstring_view, Perimeter> peris = {
                { L"inner", Perimeter::Inner }, { L"outer", Perimeter::Outer }
            };

            const auto foundValue = peris.find(value.Lower().ToStdWstring());
            return ((foundValue != peris.cend()) ? std::optional<Perimeter>(foundValue->second) :
                                                   std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::Histogram::BinningMethod>
        ConvertBinningMethod(const wxString& value)
            {
            static const std::map<std::wstring, Graphs::Histogram::BinningMethod> binMethods = {
                { L"bin-by-integer-range", Graphs::Histogram::BinningMethod::BinByIntegerRange },
                { L"bin-by-range", Graphs::Histogram::BinningMethod::BinByRange },
                { L"bin-unique-values", Graphs::Histogram::BinningMethod::BinUniqueValues }
            };

            const auto foundValue = binMethods.find(value.Lower().ToStdWstring());
            return ((foundValue != binMethods.cend()) ?
                        std::optional<Graphs::Histogram::BinningMethod>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::Histogram::IntervalDisplay>
        ConvertIntervalDisplay(const wxString& value)
            {
            static const std::map<std::wstring, Graphs::Histogram::IntervalDisplay> binIntervals = {
                { L"cutpoints", Graphs::Histogram::IntervalDisplay::Cutpoints },
                { L"midpoints", Graphs::Histogram::IntervalDisplay::Midpoints }
            };

            const auto foundValue = binIntervals.find(value.Lower().ToStdWstring());
            return ((foundValue != binIntervals.cend()) ?
                        std::optional<Graphs::Histogram::IntervalDisplay>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<RoundingMethod> ConvertRoundingMethod(const wxString& value)
            {
            static const std::map<std::wstring, RoundingMethod> roundingMethods = {
                { L"no-rounding", RoundingMethod::NoRounding },
                { L"round", RoundingMethod::Round },
                { L"round-down", RoundingMethod::RoundDown },
                { L"round-up", RoundingMethod::RoundUp }
            };

            const auto foundValue = roundingMethods.find(value.Lower().ToStdWstring());
            return ((foundValue != roundingMethods.cend()) ?
                        std::optional<RoundingMethod>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<BinLabelDisplay> ConvertBinLabelDisplay(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, BinLabelDisplay> bDisplayValues = {
                { L"percentage", BinLabelDisplay::BinPercentage },
                { L"value", BinLabelDisplay::BinValue },
                { L"value-and-percentage", BinLabelDisplay::BinValueAndPercentage },
                { L"no-display", BinLabelDisplay::NoDisplay },
                { L"bin-name", BinLabelDisplay::BinName },
                { L"bin-name-and-value", BinLabelDisplay::BinNameAndValue },
                { L"bin-name-and-percentage", BinLabelDisplay::BinNameAndPercentage },
            };

            const auto foundValue = bDisplayValues.find(value.Lower().ToStdWstring());
            return ((foundValue != bDisplayValues.cend()) ?
                        std::optional<BinLabelDisplay>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<NumberDisplay> ConvertNumberDisplay(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, NumberDisplay> bDisplayValues = {
                { L"percentage", NumberDisplay::Percentage },
                { L"value", NumberDisplay::Value },
                { L"currency", NumberDisplay::Currency }
            };

            const auto foundValue = bDisplayValues.find(value.Lower().ToStdWstring());
            return ((foundValue != bDisplayValues.cend()) ?
                        std::optional<NumberDisplay>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::Roadmap::LaneSeparatorStyle>
        ConvertLaneSeparatorStyle(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, Graphs::Roadmap::LaneSeparatorStyle>
                bDisplayValues = {
                    { L"single-line", Graphs::Roadmap::LaneSeparatorStyle::SingleLine },
                    { L"double-line", Graphs::Roadmap::LaneSeparatorStyle::DoubleLine },
                    { L"no-display", Graphs::Roadmap::LaneSeparatorStyle::NoDisplay }
                };

            const auto foundValue = bDisplayValues.find(value.Lower().ToStdWstring());
            return ((foundValue != bDisplayValues.cend()) ?
                        std::optional<Graphs::Roadmap::LaneSeparatorStyle>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::Roadmap::RoadStopTheme>
        ConvertRoadStopTheme(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, Graphs::Roadmap::RoadStopTheme> bDisplayValues = {
                { L"location-markers", Graphs::Roadmap::RoadStopTheme::LocationMarkers },
                { L"road-signs", Graphs::Roadmap::RoadStopTheme::RoadSigns }
            };

            const auto foundValue = bDisplayValues.find(value.Lower().ToStdWstring());
            return ((foundValue != bDisplayValues.cend()) ?
                        std::optional<Graphs::Roadmap::RoadStopTheme>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::Roadmap::MarkerLabelDisplay>
        ConvertMarkerLabelDisplay(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, Graphs::Roadmap::MarkerLabelDisplay>
                bDisplayValues = { { L"name", Graphs::Roadmap::MarkerLabelDisplay::Name },
                                   { L"name-and-absolute-value",
                                     Graphs::Roadmap::MarkerLabelDisplay::NameAndAbsoluteValue },
                                   { L"name-and-value",
                                     Graphs::Roadmap::MarkerLabelDisplay::NameAndValue } };

            const auto foundValue = bDisplayValues.find(value.Lower().ToStdWstring());
            return ((foundValue != bDisplayValues.cend()) ?
                        std::optional<Graphs::Roadmap::MarkerLabelDisplay>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<AxisType> ConvertAxisType(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, AxisType> axisValues = {
                { L"bottom-x", AxisType::BottomXAxis },
                { L"top-x", AxisType::TopXAxis },
                { L"left-y", AxisType::LeftYAxis },
                { L"right-y", AxisType::RightYAxis }
            };

            const auto foundValue = axisValues.find(value.Lower().ToStdWstring());
            return ((foundValue != axisValues.cend()) ?
                        std::optional<AxisType>(foundValue->second) :
                        std::nullopt);
            }

        //---------------------------------------------------
        [[nodiscard]]
        static std::optional<Graphs::BarChart::BarShape> ConvertBarShape(const wxString& value)
            {
            // use standard string, wxString should not be constructed globally
            static const std::map<std::wstring, Graphs::BarChart::BarShape> barShapeValues = {
                { L"rectangle", Graphs::BarChart::BarShape::Rectangle },
                { L"arrow", Graphs::BarChart::BarShape::Arrow },
                { L"reverse-arrow", Graphs::BarChart::BarShape::ReverseArrow }
            };

            const auto foundValue = barShapeValues.find(value.Lower().ToStdWstring());
            return ((foundValue != barShapeValues.cend()) ?
                        std::optional<Graphs::BarChart::BarShape>(foundValue->second) :
                        std::nullopt);
            }
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_ENUM_CONVERT_H
